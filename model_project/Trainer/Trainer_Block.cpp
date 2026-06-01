#include "Trainer_Block.h"

// ============================================================================
// 构造函数
// ============================================================================

Trainer_Block::Trainer_Block(const std::string& name)
    : Block(name)
    , m_TrainLength(100)
    , m_Count(0)
{
}

// ============================================================================
// Setup
// ============================================================================

bool Trainer_Block::Setup()
{
    Block::Setup();
    while (!m_outputQueue.empty()) m_outputQueue.pop();
    return true;
}

// ============================================================================
// Run — 分发
// ============================================================================

bool Trainer_Block::Run()
{
    if (IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

// ============================================================================
// DataStreamRun — 批量模式
// ============================================================================

bool Trainer_Block::DataStreamRun()
{
    std::string trainPort    = GetInputPortName(0);
    std::string decisionPort = GetInputPortName(1);
    std::string outputPort   = GetOutputPortName(0);

    auto trainData    = ReadInputData<double>(trainPort);
    auto decisionData = ReadInputData<double>(decisionPort);

    if (trainData.empty() || decisionData.empty()) return true;

    std::vector<double> outputData;

    if (m_Count < m_TrainLength)
    {
        outputData.push_back(trainData[0]);
        ++m_Count;
    }
    else
    {
        outputData.push_back(decisionData[0]);
    }

    WriteOutputData(outputPort, outputData);

    return true;
}

// ============================================================================
// TimeDrivenRun — 逐点累积模式
// ============================================================================

bool Trainer_Block::TimeDrivenRun()
{
    std::string trainPort    = GetInputPortName(0);
    std::string decisionPort = GetInputPortName(1);
    std::string outputPort   = GetOutputPortName(0);

    auto trainData    = ReadInputData<double>(trainPort);
    auto decisionData = ReadInputData<double>(decisionPort);

    for (size_t i = 0; i < trainData.size(); ++i)
        m_trainBuffer.push_back(trainData[i]);
    for (size_t i = 0; i < decisionData.size(); ++i)
        m_decisionBuffer.push_back(decisionData[i]);

    if (static_cast<int>(m_trainBuffer.size()) >= 1
        && static_cast<int>(m_decisionBuffer.size()) >= 1)
    {
        if (m_Count < m_TrainLength)
        {
            m_outputQueue.push(m_trainBuffer[0]);
            ++m_Count;
        }
        else
        {
            m_outputQueue.push(m_decisionBuffer[0]);
        }

        m_trainBuffer.clear();
        m_decisionBuffer.clear();
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

bool Trainer_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_Trainer = std::make_unique<Trainer>();

    try
    {
        m_TrainLength = std::stoi(getParameter("TrainLength").Value);
    }
    catch (...) {}

    if (m_TrainLength < 0)
    {
        LOG_ERROR("TrainLength must be > 0");
        return false;
    }

    m_Count = 0;

    AddInputPort("train",    m_Trainer->train,    1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddInputPort("decision", m_Trainer->decision, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("output",  m_Trainer->output,   1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);

    return true;
}
