#include "OrderTwoInt_Block.h"

// ============================================================================
// 构造函数
// ============================================================================

OrderTwoInt_Block::OrderTwoInt_Block(const std::string& name)
    : Block(name)
{
}

// ============================================================================
// Setup
// ============================================================================

bool OrderTwoInt_Block::Setup()
{
    Block::Setup();
    while (!m_greaterQueue.empty()) m_greaterQueue.pop();
    while (!m_lesserQueue.empty())  m_lesserQueue.pop();
    return true;
}

// ============================================================================
// Run — 分发
// ============================================================================

bool OrderTwoInt_Block::Run()
{
    if (IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

// ============================================================================
// DataStreamRun — 批量模式
// ============================================================================

bool OrderTwoInt_Block::DataStreamRun()
{
    std::string upperPort  = GetInputPortName(0);
    std::string lowerPort  = GetInputPortName(1);
    std::string greaterPort = GetOutputPortName(0);
    std::string lesserPort  = GetOutputPortName(1);

    auto upperData = ReadInputData<bool>(upperPort);
    auto lowerData = ReadInputData<bool>(lowerPort);

    if (upperData.empty() || lowerData.empty()) return true;

    std::vector<bool> greaterData;
    std::vector<bool> lesserData;

    greaterData.push_back(std::max(upperData[0], lowerData[0]));
    lesserData.push_back(std::min(upperData[0], lowerData[0]));

    WriteOutputData(greaterPort, greaterData);
    WriteOutputData(lesserPort,  lesserData);

    return true;
}

// ============================================================================
// TimeDrivenRun — 逐点累积模式
// ============================================================================

bool OrderTwoInt_Block::TimeDrivenRun()
{
    std::string upperPort  = GetInputPortName(0);
    std::string lowerPort  = GetInputPortName(1);
    std::string greaterPort = GetOutputPortName(0);
    std::string lesserPort  = GetOutputPortName(1);

    auto upperData = ReadInputData<bool>(upperPort);
    auto lowerData = ReadInputData<bool>(lowerPort);

    for (size_t i = 0; i < upperData.size(); ++i)
        m_upperBuffer.push_back(upperData[i]);
    for (size_t i = 0; i < lowerData.size(); ++i)
        m_lowerBuffer.push_back(lowerData[i]);

    if (static_cast<int>(m_upperBuffer.size()) >= 1
        && static_cast<int>(m_lowerBuffer.size()) >= 1)
    {
        m_greaterQueue.push(std::max(m_upperBuffer[0], m_lowerBuffer[0]));
        m_lesserQueue.push(std::min(m_upperBuffer[0], m_lowerBuffer[0]));

        m_upperBuffer.clear();
        m_lowerBuffer.clear();
    }

    if (!m_greaterQueue.empty() && !m_lesserQueue.empty())
    {
        bool greaterVal = m_greaterQueue.front();
        bool lesserVal  = m_lesserQueue.front();
        m_greaterQueue.pop();
        m_lesserQueue.pop();

        std::vector<bool> greaterData;
        std::vector<bool> lesserData;
        greaterData.push_back(greaterVal);
        lesserData.push_back(lesserVal);

        WriteOutputData(greaterPort, greaterData);
        WriteOutputData(lesserPort,  lesserData);
    }

    return true;
}

// ============================================================================
// Initialize
// ============================================================================

bool OrderTwoInt_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_OrderTwoInt = std::make_unique<OrderTwoInt>();

    AddInputPort("upper",  m_OrderTwoInt->upper,  1, Block::DataType::CIRCULAR_BUFFER_BOOL);
    AddInputPort("lower",  m_OrderTwoInt->lower,  1, Block::DataType::CIRCULAR_BUFFER_BOOL);
    AddOutputPort("greater", m_OrderTwoInt->greater, 1, Block::DataType::CIRCULAR_BUFFER_BOOL);
    AddOutputPort("lesser",  m_OrderTwoInt->lesser,  1, Block::DataType::CIRCULAR_BUFFER_BOOL);

    return true;
}
