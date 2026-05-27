#include "SwitchSPDT_Block.h"

#include <cmath>
#include <complex>
#include <vector>

// ============================================================================
// 构造函数
// ============================================================================

SwitchSPDT_Block::SwitchSPDT_Block(const std::string& name)
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

void SwitchSPDT_Block::SetDefaultParameters()
{
    m_Loss1      = 0.0;
    m_Isolation1 = 200.0;
    m_Loss2      = 0.0;
    m_Isolation2 = 200.0;
    m_VThreshold = 0.5;
    m_TOn1       = 0.0;
    m_TOff1      = 0.0;
    m_TOn2       = 0.0;
    m_TOff2      = 0.0;

    m_SwitchState = false;
    m_Ts          = 0.0;
}

// ============================================================================
// 参数同步到算法实例
// ============================================================================

void SwitchSPDT_Block::SetParameters()
{
    if (!m_algo) { return; }

    m_algo->Loss1      = m_Loss1;
    m_algo->Isolation1 = m_Isolation1;
    m_algo->Loss2      = m_Loss2;
    m_algo->Isolation2 = m_Isolation2;
    m_algo->VThreshold = m_VThreshold;
    m_algo->TOn1       = m_TOn1;
    m_algo->TOff1      = m_TOff1;
    m_algo->TOn2       = m_TOn2;
    m_algo->TOff2      = m_TOff2;
}

// ============================================================================
// Setup / Run / Initialize
// ============================================================================

bool SwitchSPDT_Block::Setup()
{
    Block::Setup();
    return true;
}

bool SwitchSPDT_Block::Run()
{
    return DataStreamRun();
}

bool SwitchSPDT_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_algo = std::make_unique<SwitchSPDT>();
    SetDefaultParameters();

    // ---- 读取参数 ----
    try { m_Loss1      = std::stod(getParameter("Loss1").Value);      } catch (...) {}
    try { m_Isolation1 = std::stod(getParameter("Isolation1").Value); } catch (...) {}
    try { m_Loss2      = std::stod(getParameter("Loss2").Value);      } catch (...) {}
    try { m_Isolation2 = std::stod(getParameter("Isolation2").Value); } catch (...) {}
    try { m_VThreshold = std::stod(getParameter("VThreshold").Value); } catch (...) {}
    try { m_TOn1       = std::stod(getParameter("TOn1").Value);       } catch (...) {}
    try { m_TOff1      = std::stod(getParameter("TOff1").Value);      } catch (...) {}
    try { m_TOn2       = std::stod(getParameter("TOn2").Value);       } catch (...) {}
    try { m_TOff2      = std::stod(getParameter("TOff2").Value);      } catch (...) {}

    SetParameters();

    // ---- 获取仿真参数 ----
    m_simuParam = getSimu();
    m_sampleRate = m_simuParam.samplingRate;
    m_sampleCount = 0;

    // ---- 注册端口 ----
    AddInputPort("input",   m_algo->input,   1, Block::DataType::ENVELOPE_SIGNAL);
    AddInputPort("control", m_algo->control, 1, Block::DataType::ENVELOPE_SIGNAL);
    AddOutputPort("output1", m_algo->output1, 1, Block::DataType::ENVELOPE_SIGNAL);
    AddOutputPort("output2", m_algo->output2, 1, Block::DataType::ENVELOPE_SIGNAL);

    return true;
}

// ============================================================================
// DataStreamRun：核心运行逻辑（单刀双掷开关）
// ============================================================================

bool SwitchSPDT_Block::DataStreamRun()
{
    SetParameters();

    auto inputData   = ReadInputData<EnvelopeSignal>(GetInputPortName(0));
    auto controlData = ReadInputData<EnvelopeSignal>(GetInputPortName(1));

    if (inputData.empty() || controlData.empty()) { return false; }

    // 计算当前仿真时间（参考 BPF_Butterworth 算法）
    const double t = m_simuParam.startTime + static_cast<double>(m_sampleCount) / m_sampleRate;

    // 控制信号判断开关状态
    const bool controlHigh = controlData[0].real() > m_VThreshold;

    std::vector<EnvelopeSignal> out1_vec;
    std::vector<EnvelopeSignal> out2_vec;
    out1_vec.reserve(1);
    out2_vec.reserve(1);

    const std::complex<double> x = inputData[0].complex();

    if (controlHigh) {
        // 控制高电平：output1 导通，output2 隔离
        if (!m_SwitchState) {
            m_SwitchState = true;
            m_Ts          = t;
        }

        std::complex<double> y1;
        if (t >= m_Ts + m_TOn1) {
            // 完全导通
            y1 = std::pow(10.0, -(m_Loss1 / 20.0)) * x;
        } else {
            // 过渡期：线性插值
            const double gainOn  = std::pow(10.0, -(m_Loss1 / 20.0));
            const double gainOff = std::pow(10.0, -(m_Isolation1 / 20.0));
            y1 = (gainOn - gainOff) * (t - m_Ts) / m_TOn1 * x + gainOff * x;
        }
        out1_vec.push_back(EnvelopeSignal(y1));

        std::complex<double> y2;
        if (t >= m_Ts + m_TOff2) {
            // 完全隔离
            y2 = std::pow(10.0, -(m_Isolation2 / 20.0)) * x;
        } else {
            // 过渡期：从导通到隔离
            const double gainOn  = std::pow(10.0, -(m_Loss2 / 20.0));
            const double gainOff = std::pow(10.0, -(m_Isolation2 / 20.0));
            y2 = (gainOn - gainOff) * (1.0 - (t - m_Ts) / m_TOff2) * x + gainOff * x;
        }
        out2_vec.push_back(EnvelopeSignal(y2));
    } else {
        // 控制低电平：output2 导通，output1 隔离
        if (m_SwitchState) {
            m_SwitchState = false;
            m_Ts          = t;
        }

        std::complex<double> y2;
        if (t >= m_Ts + m_TOn2) {
            // 完全导通
            y2 = std::pow(10.0, -(m_Loss2 / 20.0)) * x;
        } else {
            // 过渡期：线性插值
            const double gainOn  = std::pow(10.0, -(m_Loss2 / 20.0));
            const double gainOff = std::pow(10.0, -(m_Isolation2 / 20.0));
            y2 = (gainOn - gainOff) * (t - m_Ts) / m_TOn2 * x + gainOff * x;
        }
        out2_vec.push_back(EnvelopeSignal(y2));

        std::complex<double> y1;
        if (t >= m_Ts + m_TOff1) {
            // 完全隔离
            y1 = std::pow(10.0, -(m_Isolation1 / 20.0)) * x;
        } else {
            // 过渡期：从导通到隔离
            const double gainOn  = std::pow(10.0, -(m_Loss1 / 20.0));
            const double gainOff = std::pow(10.0, -(m_Isolation1 / 20.0));
            y1 = (gainOn - gainOff) * (1.0 - (t - m_Ts) / m_TOff1) * x + gainOff * x;
        }
        out1_vec.push_back(EnvelopeSignal(y1));
    }

    WriteOutputData(GetOutputPortName(0), out1_vec);
    WriteOutputData(GetOutputPortName(1), out2_vec);

    // 更新采样计数
    m_sampleCount += static_cast<int>(inputData.size());

    return true;
}
