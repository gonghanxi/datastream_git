#include "RADAR_BinaryDetector_Block.h"

#include <algorithm>

// ============================================================================
// 构造函数
// ============================================================================

RADAR_BinaryDetector_Block::RADAR_BinaryDetector_Block(const std::string& name)
    : Block(name)
    , m_BufferSize(0)
{
}

// ============================================================================
// SetDefaultParameters
// ============================================================================

void RADAR_BinaryDetector_Block::SetDefaultParameters()
{
    m_Threshold  = 0.6;
    m_PRI        = 1e-4;
    m_SampleRate = 10e6;
}

// ============================================================================
// SetParameters — 将解析后的参数写入算法对象
// ============================================================================

void RADAR_BinaryDetector_Block::SetParameters()
{
    if (!m_algo) return;

    m_algo->Threshold  = m_Threshold;
    m_algo->PRI        = m_PRI;
    m_algo->SampleRate = m_SampleRate;
}

// ============================================================================
// Setup
// ============================================================================

bool RADAR_BinaryDetector_Block::Setup()
{
    Block::Setup();

    while (!m_outputQueue.empty()) m_outputQueue.pop();
    m_inputAccumulator.clear();

    m_BufferSize = static_cast<int>(m_PRI * m_SampleRate);

    SetParameters();
    return true;
}

// ============================================================================
// Initialize
// ============================================================================

bool RADAR_BinaryDetector_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_algo = std::make_unique<RADAR_BinaryDetector>();

    simulator_param = getSimu();
    SetDefaultParameters();

    // 解析参数
    try { m_Threshold  = std::stod(getParameter("Threshold").Value); } catch (...) {}
    try { m_PRI        = std::stod(getParameter("PRI").Value); } catch (...) {}
    try { m_SampleRate = std::stod(getParameter("SampleRate").Value); } catch (...) {}

    // ---- 参数校验 (from RADAR_BinaryDetector::Setup) ----
    bool bStatus = true;
    if (m_Threshold < 0 || m_Threshold > 1)
    {
        LOG_ERROR("Threshold must be >= 0 and <= 1");
        bStatus = false;
    }
    if (m_PRI <= 0)
    {
        LOG_ERROR("PRI must be > 0");
        bStatus = false;
    }
    if (m_SampleRate <= 0)
    {
        LOG_ERROR("SampleRate must be > 0");
        bStatus = false;
    }
    if (!bStatus) return false;

    m_BufferSize = static_cast<int>(m_PRI * m_SampleRate);

    SetParameters();

    // 端口注册（原算法 SetRate(numPRI) → Block AddPort 第三个参数）
    AddInputPort("input",  m_algo->input,  m_BufferSize, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("output", m_algo->output, m_BufferSize, Block::DataType::CIRCULAR_BUFFER_INT);

    return true;
}

// ============================================================================
// Run — 分发
// ============================================================================

bool RADAR_BinaryDetector_Block::Run()
{
    if (IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

// ============================================================================
// DataStreamRun — 数据流模式
// ============================================================================

bool RADAR_BinaryDetector_Block::DataStreamRun()
{
    if (m_BufferSize <= 0) return true;

    // ---- 读取输入（一帧 numPRI 个采样点） ----
    auto inputData = ReadInputData<double>(GetInputPortName(0));
    if (inputData.empty()) return true;

    // ---- 在帧内找最大值 ----
    double maxValue = inputData[0];
    for (int i = 1; i < m_BufferSize; ++i)
    {
        if (inputData[i] > maxValue)
            maxValue = inputData[i];
    }

    double thresholdValue = maxValue * m_Threshold;

    // ---- 门限判决：逐点二值化输出 ----
    std::vector<int> outputData(m_BufferSize);
    for (int i = 0; i < m_BufferSize; ++i)
    {
        outputData[i] = inputData[i] > thresholdValue ? 1 : 0;
    }

    WriteOutputData(GetOutputPortName(0), outputData);

    return true;
}

// ============================================================================
// TimeDrivenRun — 变步长模式
// ============================================================================

bool RADAR_BinaryDetector_Block::TimeDrivenRun()
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
        for (int i = 1; i < m_BufferSize; ++i)
        {
            if (m_inputAccumulator[i] > maxValue)
                maxValue = m_inputAccumulator[i];
        }

        double thresholdValue = maxValue * m_Threshold;

        std::vector<int> outputData(m_BufferSize);
        for (int i = 0; i < m_BufferSize; ++i)
        {
            outputData[i] = m_inputAccumulator[i] > thresholdValue ? 1 : 0;
        }

        m_outputQueue.push(std::move(outputData));
        m_inputAccumulator.erase(m_inputAccumulator.begin(), m_inputAccumulator.begin() + m_BufferSize);
    }

    // ---- ③ 出队写入 ----
    if (!m_outputQueue.empty())
    {
        WriteOutputData(GetOutputPortName(0), m_outputQueue.front());
        m_outputQueue.pop();
    }

    return true;
}
