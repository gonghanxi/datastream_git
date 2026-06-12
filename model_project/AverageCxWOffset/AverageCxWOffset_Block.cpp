#include "AverageCxWOffset_Block.h"

// ============================================================================
// 构造函数
// ============================================================================

AverageCxWOffset_Block::AverageCxWOffset_Block(const std::string& name)
    : Block(name)
    , m_NumInputsToAverage(256)
    , m_InitialZeros(0)
    , m_CurrentSum(0.0, 0.0)
    , m_CurrentAverage(0.0, 0.0)
    , m_BufferIndex(0)
{
}

// ============================================================================
// Setup
// ============================================================================

bool AverageCxWOffset_Block::Setup()
{
    Block::Setup();
    while (!m_outputQueue.empty()) m_outputQueue.pop();
    return true;
}

// ============================================================================
// Run — 分发
// ============================================================================

bool AverageCxWOffset_Block::Run()
{
    if (IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

// ============================================================================
// DataStreamRun — 批量模式
// ============================================================================

bool AverageCxWOffset_Block::DataStreamRun()
{
    std::string inputPort  = GetInputPortName(0);
    std::string offsetPort = GetInputPortName(1);
    std::string outputPort = GetOutputPortName(0);

    auto inputData  = ReadInputData<std::complex<double>>(inputPort);
    auto offsetData = ReadInputData<int>(offsetPort);

    if (inputData.empty() || offsetData.empty()) return true;

    const int offset = offsetData[0];

    if (offset <= 0)
    {
        LOG_ERROR("The offset must be non-negative.");
        return false;
    }

    std::vector<std::complex<double>> outputData;

    if (m_InitialZeros < offset)
    {
        outputData.push_back(std::complex<double>(0.0, 0.0));
        ++m_InitialZeros;
    }
    else
    {
        ++m_BufferIndex;

        if (m_BufferIndex > m_NumInputsToAverage)
        {
            m_CurrentAverage = m_CurrentSum / static_cast<double>(m_NumInputsToAverage);
            m_BufferIndex -= m_NumInputsToAverage;
            m_CurrentSum = std::complex<double>(0.0, 0.0);
        }

        m_CurrentSum += inputData[0];
        outputData.push_back(m_CurrentAverage);
    }

    WriteOutputData(outputPort, outputData);

    return true;
}

// ============================================================================
// TimeDrivenRun — 逐点累积模式
// ============================================================================

bool AverageCxWOffset_Block::TimeDrivenRun()
{
    std::string inputPort  = GetInputPortName(0);
    std::string offsetPort = GetInputPortName(1);
    std::string outputPort = GetOutputPortName(0);

    auto inputData  = ReadInputData<std::complex<double>>(inputPort);
    auto offsetData = ReadInputData<int>(offsetPort);

    for (size_t i = 0; i < inputData.size(); ++i)
        m_inputBuffer.push_back(inputData[i]);
    for (size_t i = 0; i < offsetData.size(); ++i)
        m_offsetBuffer.push_back(offsetData[i]);

    if (static_cast<int>(m_inputBuffer.size()) >= 1
        && static_cast<int>(m_offsetBuffer.size()) >= 1)
    {
        const int offset = m_offsetBuffer[0];

        if (offset <= 0)
        {
            LOG_ERROR("The offset must be non-negative.");
            return false;
        }

        if (m_InitialZeros < offset)
        {
            m_outputQueue.push(std::complex<double>(0.0, 0.0));
            ++m_InitialZeros;
        }
        else
        {
            ++m_BufferIndex;

            if (m_BufferIndex > m_NumInputsToAverage)
            {
                m_CurrentAverage = m_CurrentSum / static_cast<double>(m_NumInputsToAverage);
                m_BufferIndex -= m_NumInputsToAverage;
                m_CurrentSum = std::complex<double>(0.0, 0.0);
            }

            m_CurrentSum += m_inputBuffer[0];
            m_outputQueue.push(m_CurrentAverage);
        }

        m_inputBuffer.clear();
        m_offsetBuffer.clear();
    }

    if (!m_outputQueue.empty())
    {
        std::complex<double> val = m_outputQueue.front();
        m_outputQueue.pop();

        std::vector<std::complex<double>> outputData;
        outputData.push_back(val);
        WriteOutputData(outputPort, outputData);
    }

    return true;
}

// ============================================================================
// Initialize
// ============================================================================

bool AverageCxWOffset_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_AverageCxWOffset = std::make_unique<AverageCxWOffset>();

    SetDefaultParameters();
    try { m_NumInputsToAverage = std::stoi(getParameter("NumInputsToAverage").Value); } catch(...) { LOG_WARN("Failed to parse parameter 'NumInputsToAverage', using default value."); }
    SetParameters();
    if (!m_AverageCxWOffset->Setup()) return false;

    AddInputPort("input",  m_AverageCxWOffset->input,  1, Block::DataType::CIRCULAR_BUFFER_DCOMPLEX);
    AddInputPort("Offset", m_AverageCxWOffset->Offset, 1, Block::DataType::CIRCULAR_BUFFER_INT);
    AddOutputPort("output", m_AverageCxWOffset->output, 1, Block::DataType::CIRCULAR_BUFFER_DCOMPLEX);

    return true;
}

// ============================================================================
// 参数设置
// ============================================================================

void AverageCxWOffset_Block::SetDefaultParameters()
{
    m_NumInputsToAverage = 256;

    m_InitialZeros   = 0;
    m_CurrentSum     = std::complex<double>(0.0, 0.0);
    m_CurrentAverage = std::complex<double>(0.0, 0.0);
    m_BufferIndex    = 0;
}

void AverageCxWOffset_Block::SetParameters()
{
    if (!m_AverageCxWOffset) return;
    m_AverageCxWOffset->NumInputsToAverage = m_NumInputsToAverage;
}
