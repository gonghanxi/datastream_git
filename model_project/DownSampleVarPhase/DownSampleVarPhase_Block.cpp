#include "DownSampleVarPhase_Block.h"

// ============================================================================
// 构造函数
// ============================================================================

DownSampleVarPhase_Block::DownSampleVarPhase_Block(const std::string& name)
    : Block(name)
    , m_Factor(2)
{
}

// ============================================================================
// Setup
// ============================================================================

bool DownSampleVarPhase_Block::Setup()
{
    Block::Setup();
    return true;
}

// ============================================================================
// Run — 分发
// ============================================================================

bool DownSampleVarPhase_Block::Run()
{
    if (IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

// ============================================================================
// DataStreamRun — 批量模式
// ============================================================================

bool DownSampleVarPhase_Block::DataStreamRun()
{
    std::string inputPort  = GetInputPortName(0);
    std::string phasePort  = GetInputPortName(1);
    std::string outputPort = GetOutputPortName(0);

    auto inputData = ReadInputData<double>(inputPort);
    auto phaseData = ReadInputData<int>(phasePort);

    const int expectedSize = m_Factor;
    if (static_cast<int>(inputData.size()) < expectedSize) return true;
    if (phaseData.empty()) return true;

    int phase = phaseData[0];
    if (phase < 0) phase = 0;
    if (phase >= m_Factor) phase = m_Factor - 1;

    std::vector<double> outputData;
    outputData.push_back(inputData[static_cast<size_t>(phase)]);

    WriteOutputData(outputPort, outputData);

    return true;
}

// ============================================================================
// TimeDrivenRun — 逐点累积模式
// ============================================================================

bool DownSampleVarPhase_Block::TimeDrivenRun()
{
    std::string inputPort  = GetInputPortName(0);
    std::string phasePort  = GetInputPortName(1);
    std::string outputPort = GetOutputPortName(0);

    auto inputData = ReadInputData<double>(inputPort);
    auto phaseData = ReadInputData<int>(phasePort);

    for (size_t i = 0; i < inputData.size(); ++i)
        m_inputBuffer.push_back(inputData[i]);
    for (size_t i = 0; i < phaseData.size(); ++i)
        m_phaseBuffer.push_back(phaseData[i]);

    if (static_cast<int>(m_inputBuffer.size()) >= m_Factor && !m_phaseBuffer.empty())
    {
        int phase = m_phaseBuffer[0];
        if (phase < 0) phase = 0;
        if (phase >= m_Factor) phase = m_Factor - 1;

        m_outputQueue.push(m_inputBuffer[static_cast<size_t>(phase)]);

        m_inputBuffer.clear();
        m_phaseBuffer.clear();
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

bool DownSampleVarPhase_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_DownSampleVarPhase = std::make_unique<DownSampleVarPhase>();

    SetDefaultParameters();

    try { m_Factor = std::stoi(getParameter("Factor").Value); } catch (...) {}

    SetParameters();

    if (m_Factor < 1)
    {
        LOG_ERROR("Factor must be >= 1.");
        return false;
    }

    AddInputPort("input", m_DownSampleVarPhase->input, static_cast<size_t>(m_Factor), Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddInputPort("phase", m_DownSampleVarPhase->phase, 1, Block::DataType::CIRCULAR_BUFFER_INT);
    AddOutputPort("output", m_DownSampleVarPhase->output, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);

    return true;
}

// ============================================================================
// 参数设置
// ============================================================================

void DownSampleVarPhase_Block::SetDefaultParameters()
{
    m_Factor = 2;
}

void DownSampleVarPhase_Block::SetParameters()
{
    if (!m_DownSampleVarPhase) return;
    m_DownSampleVarPhase->Factor = m_Factor;
}
