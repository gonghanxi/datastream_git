#include "SwitchSPST_Block.h"

#include <cmath>
#include <complex>
#include <vector>

// ============================================================================
// 构造函数
// ============================================================================

SwitchSPST_Block::SwitchSPST_Block(const std::string& name)
    : Block(name)
    , m_SwitchState(false)
    , m_Ts(0.0)
    , m_sampleCount(0)
    , m_sampleRate(0.0)
{
}

// ============================================================================
// 默认参数
// ============================================================================

void SwitchSPST_Block::SetDefaultParameters()
{
    m_Loss      = 0.0;
    m_Isolation = 200.0;
    m_VThreshold = 0.5;
    m_TOn       = 0.0;
    m_TOff      = 0.0;

    m_SwitchState = false;
    m_Ts          = 0.0;
}

// ============================================================================
// 参数同步到算法实例
// ============================================================================

void SwitchSPST_Block::SetParameters()
{
    if (!m_algo) { return; }

    m_algo->Loss      = m_Loss;
    m_algo->Isolation = m_Isolation;
    m_algo->VThreshold = m_VThreshold;
    m_algo->TOn       = m_TOn;
    m_algo->TOff      = m_TOff;
}

// ============================================================================
// Setup / Run / Initialize
// ============================================================================

bool SwitchSPST_Block::Setup()
{
    Block::Setup();

    bool bStatus = true;

    if (m_VThreshold <= 0)
    {
        LOG_ERROR("VThreshold must be > 0");
        bStatus = false;
    }
    if (m_TOn < 0)
    {
        LOG_ERROR("TOn must be >= 0");
        bStatus = false;
    }
    if (m_TOff < 0)
    {
        LOG_ERROR("TOff must be >= 0");
        bStatus = false;
    }

    return bStatus;
}

bool SwitchSPST_Block::Run()
{
    return DataStreamRun();
}

bool SwitchSPST_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_algo = std::make_unique<SwitchSPST>();
    SetDefaultParameters();

    // ---- 读取参数 ----
    try { m_Loss      = std::stod(getParameter("Loss").Value);      } catch (...) {}
    try { m_Isolation = std::stod(getParameter("Isolation").Value); } catch (...) {}
    try { m_VThreshold = std::stod(getParameter("VThreshold").Value); } catch (...) {}
    try { m_TOn       = std::stod(getParameter("TOn").Value);       } catch (...) {}
    try { m_TOff      = std::stod(getParameter("TOff").Value);      } catch (...) {}

    SetParameters();

    // ---- 获取仿真参数 ----
    m_simuParam = getSimu();
    m_sampleRate = m_simuParam.samplingRate;
    m_sampleCount = 0;

    // ---- 注册端口 ----
    AddInputPort("input",   m_algo->input,   1, Block::DataType::ENVELOPE_SIGNAL);
    AddInputPort("control", m_algo->control, 1, Block::DataType::ENVELOPE_SIGNAL);
    AddOutputPort("output", m_algo->output,  1, Block::DataType::ENVELOPE_SIGNAL);

    return true;
}

// ============================================================================
// DataStreamRun：核心运行逻辑（单刀单掷开关）
// ============================================================================

bool SwitchSPST_Block::DataStreamRun()
{
    auto inputData   = ReadInputData<EnvelopeSignal>(GetInputPortName(0));
    auto controlData = ReadInputData<EnvelopeSignal>(GetInputPortName(1));

    if (inputData.empty() || controlData.empty()) { return false; }

    // 计算当前仿真时间（参考 BPF_Butterworth 算法）
    const double t = m_simuParam.startTime + static_cast<double>(m_sampleCount) / m_sampleRate;

    // 控制信号判断开关状态
    const double controlVoltage = controlData[0].real();

    std::vector<EnvelopeSignal> out_vec;
    out_vec.reserve(1);

    const std::complex<double> x = inputData[0].complex();

    if (controlVoltage > m_VThreshold) {
        // 控制高电平：开关导通
        if (!m_SwitchState) {
            m_SwitchState = true;
            m_Ts          = t;
        }

        std::complex<double> y;
        if (t >= m_Ts + m_TOn) {
            // 完全导通
            y = std::pow(10.0, -(m_Loss / 20.0)) * x;
        } else {
            // 过渡期：线性插值
            const double gainOn  = std::pow(10.0, -(m_Loss / 20.0));
            const double gainOff = std::pow(10.0, -(m_Isolation / 20.0));
            y = (gainOn - gainOff) * (t - m_Ts) / m_TOn * x + gainOff * x;
        }
        out_vec.push_back(EnvelopeSignal(y));
    } else {
        // 控制低电平：开关断开
        if (m_SwitchState) {
            m_SwitchState = false;
            m_Ts          = t;
        }

        std::complex<double> y;
        if (t >= m_Ts + m_TOff) {
            // 完全隔离
            y = std::pow(10.0, -(m_Isolation / 20.0)) * x;
        } else {
            // 过渡期：从导通到隔离
            const double gainOn  = std::pow(10.0, -(m_Loss / 20.0));
            const double gainOff = std::pow(10.0, -(m_Isolation / 20.0));
            y = (gainOn - gainOff) * (1.0 - (t - m_Ts) / m_TOff) * x + gainOff * x;
        }
        out_vec.push_back(EnvelopeSignal(y));
    }

    WriteOutputData(GetOutputPortName(0), out_vec);

    // 更新采样计数
    m_sampleCount += static_cast<int>(inputData.size());

    return true;
}
