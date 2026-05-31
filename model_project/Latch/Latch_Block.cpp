#include "Latch_Block.h"

// ============================================================================
// 构造函数
// ============================================================================

Latch_Block::Latch_Block(const std::string& name)
    : Block(name)
{
}

// ============================================================================
// Setup
// ============================================================================

bool Latch_Block::Setup()
{
    Block::Setup();
    return true;
}

// ============================================================================
// Run — 分发
// ============================================================================

bool Latch_Block::Run()
{
    if (IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

// ============================================================================
// DataStreamRun — 批量模式
// ============================================================================

bool Latch_Block::DataStreamRun()
{
    std::string inputPort  = GetInputPortName(0);
    std::string clockPort  = GetInputPortName(1);
    std::string outputPort = GetOutputPortName(0);

    auto inputData = ReadInputData<double>(inputPort);
    auto clockData = ReadInputData<int>(clockPort);

    if (inputData.empty() || clockData.empty()) return true;

    std::vector<double> outputData;
    if (clockData[0] != 0)
    {
        m_Latch->storedValue = inputData[0];
        outputData.push_back(inputData[0]);
    }
    else
    {
        outputData.push_back(m_Latch->storedValue);
    }

    WriteOutputData(outputPort, outputData);

    return true;
}

// ============================================================================
// TimeDrivenRun — 逐点累积模式
// ============================================================================

bool Latch_Block::TimeDrivenRun()
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

    if (static_cast<int>(m_inputBuffer.size()) >= 1 && static_cast<int>(m_clockBuffer.size()) >= 1)
    {
        double val;
        if (m_clockBuffer[0] != 0)
        {
            m_Latch->storedValue = m_inputBuffer[0];
            val = m_inputBuffer[0];
        }
        else
        {
            val = m_Latch->storedValue;
        }

        m_outputQueue.push(val);

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

bool Latch_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_Latch = std::make_unique<Latch>();

    SetDefaultParameters();
    SetParameters();

    AddInputPort("input", m_Latch->input, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddInputPort("clock", m_Latch->clock, 1, Block::DataType::CIRCULAR_BUFFER_INT);
    AddOutputPort("output", m_Latch->output, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);

    return true;
}

// ============================================================================
// 参数设置
// ============================================================================

void Latch_Block::SetDefaultParameters()
{
    // Latch 无用户参数，仅内部状态 storedValue
}

void Latch_Block::SetParameters()
{
    // Latch 无用户参数
}
