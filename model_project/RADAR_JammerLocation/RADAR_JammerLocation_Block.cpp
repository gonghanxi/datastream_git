#include "RADAR_JammerLocation_Block.h"

#include <algorithm>
#include <cmath>

// ============================================================================
// 构造函数
// ============================================================================

RADAR_JammerLocation_Block::RADAR_JammerLocation_Block(const std::string& name)
    : Block(name)
    , m_BufferSize(0)
    , m_inputAccumulator()
    , m_outputQueue()
{
}

// ============================================================================
// SetDefaultParameters
// ============================================================================

void RADAR_JammerLocation_Block::SetDefaultParameters()
{
    m_PRI        = 1e-4;
    m_SampleRate = 10e6;
}

// ============================================================================
// SetParameters — 将解析后的参数写入算法对象
// ============================================================================

void RADAR_JammerLocation_Block::SetParameters()
{
    if (!m_algo) return;

    m_algo->PRI        = m_PRI;
    m_algo->SampleRate = m_SampleRate;
}

// ============================================================================
// Setup
// ============================================================================

bool RADAR_JammerLocation_Block::Setup()
{
    Block::Setup();

    while (!m_outputQueue.empty()) m_outputQueue.pop();
    m_inputAccumulator.clear();

    m_BufferSize = static_cast<int>(m_PRI * m_SampleRate);

    SetParameters();
    return true;
}

// ============================================================================
// Run — 分发
// ============================================================================

bool RADAR_JammerLocation_Block::Run()
{
    if (IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

// ============================================================================
// DataStreamRun — 数据流模式
// ============================================================================

bool RADAR_JammerLocation_Block::DataStreamRun()
{
    if (m_BufferSize <= 0) return true;

    // ---- 读取输入（range-doppler 矩阵） ----
    auto inputData = ReadInputData<double>(GetInputPortName(0));
    if (inputData.empty()) return true;

    // ---- 在帧数据中找最大值的位置 ----
    const int processLen = std::min(m_BufferSize, static_cast<int>(inputData.size()));
    double maxValue = inputData[0];
    int    maxIndex = 0;

    for (int i = 1; i < processLen; ++i)
    {
        if (inputData[i] > maxValue)
        {
            maxValue = inputData[i];
            maxIndex = i;
        }
    }

    // ---- 计算距离（原算法逻辑） ----
    const double c = 3e8;
    double Range = c * (maxIndex / m_SampleRate) / 2.0;

    // ---- 写入输出 ----
    WriteOutputData(GetOutputPortName(0), std::vector<double>{Range});

    return true;
}

// ============================================================================
// TimeDrivenRun — 变步长模式
// ============================================================================

bool RADAR_JammerLocation_Block::TimeDrivenRun()
{
    if (m_BufferSize <= 0) return true;

    // ---- ① 累积输入样本 ----
    auto inputData = ReadInputData<double>(GetInputPortName(0));
    if (inputData.empty()) return true;

    for (const auto& val : inputData)
        m_inputAccumulator.push_back(val);

    // ---- ② 判断阈值：收齐一帧数据 ----
    if (static_cast<int>(m_inputAccumulator.size()) >= m_BufferSize)
    {
        double maxValue = m_inputAccumulator[0];
        int    maxIndex = 0;

        for (int i = 1; i < m_BufferSize; ++i)
        {
            if (m_inputAccumulator[i] > maxValue)
            {
                maxValue = m_inputAccumulator[i];
                maxIndex = i;
            }
        }

        const double c = 3e8;
        double Range = c * (maxIndex / m_SampleRate) / 2.0;

        m_outputQueue.push(Range);
        m_inputAccumulator.clear();
    }

    // ---- ③ 出队写入 ----
    if (!m_outputQueue.empty())
    {
        WriteOutputData(GetOutputPortName(0), std::vector<double>{m_outputQueue.front()});
        m_outputQueue.pop();
    }

    return true;
}

// ============================================================================
// Initialize
// ============================================================================

bool RADAR_JammerLocation_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_algo = std::make_unique<RADAR_JammerLocation>();

    simulator_param = getSimu();
    SetDefaultParameters();

    // 解析参数
    try { m_PRI        = std::stod(getParameter("PRI").Value); } catch (...) {}
    try { m_SampleRate = std::stod(getParameter("SampleRate").Value); } catch (...) {}

    // ---- 参数校验 (from RADAR_JammerLocation::Setup) ----
    if (m_PRI * m_SampleRate <= 0)
    {
        LOG_ERROR("input port rate PRI * SampleRate must be greater than 0.");
        return false;
    }

    m_BufferSize = static_cast<int>(m_PRI * m_SampleRate);

    SetParameters();

    // 端口注册
    AddInputPort("input",  m_algo->input,  m_BufferSize, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("Range", m_algo->Range,  1,            Block::DataType::CIRCULAR_BUFFER_DOUBLE);

    return true;
}
