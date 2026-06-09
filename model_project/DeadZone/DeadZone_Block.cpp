#include "DeadZone_Block.h"

// ============================================================================
// 构造函数
// ============================================================================

DeadZone_Block::DeadZone_Block(const std::string& name)
    : Block(name)
{
}

// ============================================================================
// Setup
// ============================================================================

bool DeadZone_Block::Setup()
{
    Block::Setup();
    while (!m_outputQueue.empty()) m_outputQueue.pop();
    return true;
}

// ============================================================================
// Run — 分发
// ============================================================================

bool DeadZone_Block::Run()
{
    return DataStreamRun();
}

// ============================================================================
// DataStreamRun — 批量模式
// ============================================================================

bool DeadZone_Block::DataStreamRun()
{
    std::string inputPort  = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    auto inputData = ReadInputData<double>(inputPort);

    if (inputData.empty()) return true;

    const double x = inputData[0];

    std::vector<double> outputData;

    if (x > m_High)
    {
        outputData.push_back(m_K * (x - m_High));
    }
    else if (x < m_Low)
    {
        outputData.push_back(m_K * (x - m_Low));
    }
    else
    {
        outputData.push_back(0.0);
    }

    WriteOutputData(outputPort, outputData);

    return true;
}

// ============================================================================
// Initialize
// ============================================================================

bool DeadZone_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_DeadZone = std::make_unique<DeadZone>();

    SetDefaultParameters();
    try { m_K    = std::stod(getParameter("K").Value);    } catch(...) {}
    try { m_Low  = std::stod(getParameter("Low").Value);  } catch(...) {}
    try { m_High = std::stod(getParameter("High").Value); } catch(...) {}
    SetParameters();

    if (m_Low >= m_High)
    {
        LOG_ERROR("Low must be smaller than High.");
        return false;
    }

    AddInputPort("input",  m_DeadZone->input,  1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("output", m_DeadZone->output, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);

    return true;
}

// ============================================================================
// 参数设置
// ============================================================================

void DeadZone_Block::SetDefaultParameters()
{
    m_K    = 1.0;
    m_Low  = 0.0;
    m_High = 1.0;
}

void DeadZone_Block::SetParameters()
{
    if (!m_DeadZone) return;
    m_DeadZone->K    = m_K;
    m_DeadZone->Low  = m_Low;
    m_DeadZone->High = m_High;
}
