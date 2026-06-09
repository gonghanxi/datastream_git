#include "SchmittTrig_Block.h"

// ============================================================================
// 构造函数
// ============================================================================

SchmittTrig_Block::SchmittTrig_Block(const std::string& name)
    : Block(name)
{
}

// ============================================================================
// Setup
// ============================================================================

bool SchmittTrig_Block::Setup()
{
    Block::Setup();
    while (!m_outputQueue.empty()) m_outputQueue.pop();
    m_inputBuffer.clear();
    return true;
}

// ============================================================================
// Run — 分发
// ============================================================================

bool SchmittTrig_Block::Run()
{
    return DataStreamRun();
}

// ============================================================================
// DataStreamRun — 批量模式
// ============================================================================

bool SchmittTrig_Block::DataStreamRun()
{
    std::string inputPort  = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    auto inputData = ReadInputData<double>(inputPort);
    if (inputData.empty()) return true;

    const double x = inputData[0];

    if (x > m_IHigh)
        m_TrigStatus = true;
    if (x < m_ILow)
        m_TrigStatus = false;

    std::vector<double> outputData;
    outputData.push_back(m_TrigStatus ? m_OHigh : m_OLow);
    WriteOutputData(outputPort, outputData);

    return true;
}

// ============================================================================
// Initialize
// ============================================================================

bool SchmittTrig_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_SchmittTrig = std::make_unique<SchmittTrig>();

    // 解析参数
    try { m_ILow  = std::stod(getParameter("ILow").Value);  } catch (...) {}
    try { m_IHigh = std::stod(getParameter("IHigh").Value); } catch (...) {}
    try { m_OLow  = std::stod(getParameter("OLow").Value);  } catch (...) {}
    try { m_OHigh = std::stod(getParameter("OHigh").Value); } catch (...) {}

    if (m_ILow > m_IHigh)
    {
        LOG_ERROR("SchmittTrig: ILow must be <= IHigh.");
        return false;
    }
    if (m_OLow > m_OHigh)
    {
        LOG_ERROR("SchmittTrig: OLow must be <= OHigh.");
        return false;
    }

    m_TrigStatus = false;

    AddInputPort("input",  m_SchmittTrig->input,  1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("output", m_SchmittTrig->output, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);

    return true;
}
