#include "Mux_Block.h"

// ============================================================================
// 构造函数
// ============================================================================

Mux_Block::Mux_Block(const std::string& name)
    : Block(name)
    , m_BlockSize(1)
{
}

// ============================================================================
// Setup
// ============================================================================

bool Mux_Block::Setup()
{
    Block::Setup();
    while (!m_outputQueue.empty()) m_outputQueue.pop();
    return true;
}

// ============================================================================
// Run — 分发
// ============================================================================

bool Mux_Block::Run()
{
    if (IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

// ============================================================================
// DataStreamRun — 批量模式
// ============================================================================

bool Mux_Block::DataStreamRun()
{
    std::string inputPort   = GetInputPortName(0);
    std::string controlPort = GetInputPortName(1);
    std::string outputPort  = GetOutputPortName(0);

    auto inputData   = ReadInputData<double>(inputPort);
    auto controlData = ReadInputData<int>(controlPort);

    if (inputData.empty() || controlData.empty()) return true;

    int channel = controlData[0];
    size_t numChannels = inputData.size() / static_cast<size_t>(m_BlockSize);

    if (channel < 0 || channel >= static_cast<int>(numChannels))
    {
        LOG_ERROR("The control input can only accept values in the range [0, N - 1], where N is the input size.");
        return false;
    }

    std::vector<double> outputData;
    outputData.reserve(static_cast<size_t>(m_BlockSize));

    for (int i = 0; i < m_BlockSize; ++i)
    {
        outputData.push_back(inputData[static_cast<size_t>(channel * m_BlockSize + i)]);
    }

    WriteOutputData(outputPort, outputData);

    return true;
}

// ============================================================================
// TimeDrivenRun — 逐点累积模式
// ============================================================================

bool Mux_Block::TimeDrivenRun()
{
    std::string inputPort   = GetInputPortName(0);
    std::string controlPort = GetInputPortName(1);
    std::string outputPort  = GetOutputPortName(0);

    auto inputData   = ReadInputData<double>(inputPort);
    auto controlData = ReadInputData<int>(controlPort);

    if (inputData.empty() || controlData.empty()) return true;

    for (size_t i = 0; i < inputData.size(); ++i)
        m_inputBuffer.push_back(inputData[i]);
    for (size_t i = 0; i < controlData.size(); ++i)
        m_controlBuffer.push_back(controlData[i]);

    if (static_cast<int>(m_inputBuffer.size()) >= m_BlockSize
        && static_cast<int>(m_controlBuffer.size()) >= 1)
    {
        int channel = m_controlBuffer[0];
        size_t numChannels = m_inputBuffer.size() / static_cast<size_t>(m_BlockSize);

        if (channel < 0 || channel >= static_cast<int>(numChannels))
        {
            LOG_ERROR("The control input can only accept values in the range [0, N - 1], where N is the input size.");
            return false;
        }

        for (int i = 0; i < m_BlockSize; ++i)
        {
            m_outputQueue.push(m_inputBuffer[static_cast<size_t>(channel * m_BlockSize + i)]);
        }

        m_inputBuffer.clear();
        m_controlBuffer.clear();
    }

    if (!m_outputQueue.empty())
    {
        double val = m_outputQueue.front();
        m_outputQueue.pop();

        std::vector<double> outputData;
        outputData.push_back(val);
        WriteOutputData(outputPort, outputData);
    }

    return true;
}

// ============================================================================
// Initialize
// ============================================================================

bool Mux_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_Mux = std::make_unique<Mux>();

    SetDefaultParameters();

    try { m_BlockSize = std::stoi(getParameter("BlockSize").Value); } catch (...) {}

    SetParameters();

    if (m_BlockSize < 1)
    {
        LOG_ERROR("BlockSize must be >= 1.");
        return false;
    }

    AddInputPort("input",   m_Mux->input,   static_cast<size_t>(m_BlockSize), Block::DataType::DOUBLE_BUS);
    AddInputPort("control", m_Mux->control, 1, Block::DataType::CIRCULAR_BUFFER_INT);
    AddOutputPort("output", m_Mux->output,  static_cast<size_t>(m_BlockSize), Block::DataType::CIRCULAR_BUFFER_DOUBLE);

    return true;
}

// ============================================================================
// 参数设置
// ============================================================================

void Mux_Block::SetDefaultParameters()
{
    m_BlockSize = 1;
}

void Mux_Block::SetParameters()
{
    if (!m_Mux) return;
    m_Mux->BlockSize = m_BlockSize;
}
