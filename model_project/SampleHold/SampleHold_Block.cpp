#include "SampleHold_Block.h"

// ============================================================================
// 构造函数
// ============================================================================

SampleHold_Block::SampleHold_Block(const std::string& name)
    : Block(name)
{
}

// ============================================================================
// Setup
// ============================================================================

bool SampleHold_Block::Setup()
{
    Block::Setup();
    while (!m_outputQueue.empty()) m_outputQueue.pop();
    return true;
}

// ============================================================================
// Run — 分发
// ============================================================================

bool SampleHold_Block::Run()
{
    if (IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

// ============================================================================
// DataStreamRun — 批量模式
// ============================================================================

bool SampleHold_Block::DataStreamRun()
{
    std::string inputPort  = GetInputPortName(0);
    std::string clockPort  = GetInputPortName(1);
    std::string outputPort = GetOutputPortName(0);

    auto inputData = ReadInputData<double>(inputPort);
    auto clockData = ReadInputData<int>(clockPort);

    if (inputData.empty() || clockData.empty()) return true;

    if (clockData[0] != 0)
    {
        std::vector<double> outputData;
        outputData.push_back(inputData[0]);
        WriteOutputData(outputPort, outputData);
    }

    return true;
}

// ============================================================================
// TimeDrivenRun — 逐点累积模式
// ============================================================================

bool SampleHold_Block::TimeDrivenRun()
{
    std::string inputPort  = GetInputPortName(0);
    std::string clockPort  = GetInputPortName(1);
    std::string outputPort = GetOutputPortName(0);

    auto inputData = ReadInputData<double>(inputPort);
    auto clockData = ReadInputData<int>(clockPort);

    for (size_t i = 0; i < inputData.size(); ++i)
        m_inputBuffer.push_back(inputData[i]);
    for (size_t i = 0; i < clockData.size(); ++i)
        m_clockBuffer.push_back(clockData[i]);

    if (static_cast<int>(m_inputBuffer.size()) >= 1
        && static_cast<int>(m_clockBuffer.size()) >= 1)
    {
        if (m_clockBuffer[0] != 0)
        {
            m_outputQueue.push(m_inputBuffer[0]);
        }

        m_inputBuffer.clear();
        m_clockBuffer.clear();
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

bool SampleHold_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_SampleHold = std::make_unique<SampleHold>();

    AddInputPort("input", m_SampleHold->input, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddInputPort("clock", m_SampleHold->clock, 1, Block::DataType::CIRCULAR_BUFFER_INT);
    AddOutputPort("output", m_SampleHold->output, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);

    return true;
}
