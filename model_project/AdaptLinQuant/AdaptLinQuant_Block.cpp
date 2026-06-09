#include "AdaptLinQuant_Block.h"

#include <cmath>

// ============================================================================
// 构造函数
// ============================================================================

AdaptLinQuant_Block::AdaptLinQuant_Block(const std::string& name)
    : Block(name)
    , m_Bits(8)
{
}

// ============================================================================
// Setup
// ============================================================================

bool AdaptLinQuant_Block::Setup()
{
    Block::Setup();
    m_amplitudeQueue = std::queue<double>();
    m_outStepQueue   = std::queue<double>();
    m_stepLevelQueue = std::queue<int>();
    return true;
}

// ============================================================================
// Run — 分发
// ============================================================================

bool AdaptLinQuant_Block::Run()
{
    if (IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

// ============================================================================
// DataStreamRun — 批量模式
// ============================================================================

bool AdaptLinQuant_Block::DataStreamRun()
{
    std::string inputPort    = GetInputPortName(0);
    std::string inStepPort   = GetInputPortName(1);
    std::string amplitudePort = GetOutputPortName(0);
    std::string outStepPort   = GetOutputPortName(1);
    std::string stepLevelPort = GetOutputPortName(2);

    auto inputData  = ReadInputData<double>(inputPort);
    auto inStepData = ReadInputData<double>(inStepPort);

    if (inputData.empty() || inStepData.empty()) return true;

    const double step = inStepData[0];

    if (!(step > 0.0) || !std::isfinite(step))
    {
        LOG_ERROR("AdaptLinQuant: inStep must be finite and > 0.");
        return false;
    }

    const unsigned int L_u = (1u << m_Bits);
    const double       L   = static_cast<double>(L_u);
    const double halfSpanMinusHalf = 0.5 * L - 0.5;

    const double x = inputData[0];

    double kRound = std::floor(x / step + halfSpanMinusHalf + 0.5);

    if (kRound < 0.0)
        kRound = 0.0;

    const double kMax = static_cast<double>(L_u - 1u);
    if (kRound > kMax)
        kRound = kMax;

    const int k = static_cast<int>(kRound);

    const double q = (static_cast<double>(k) - halfSpanMinusHalf) * step;

    std::vector<double> amplitudeData;
    std::vector<double> outStepData;
    std::vector<int>    stepLevelData;

    amplitudeData.push_back(q);
    outStepData.push_back(step);
    stepLevelData.push_back(k);

    WriteOutputData(amplitudePort, amplitudeData);
    WriteOutputData(outStepPort,   outStepData);
    WriteOutputData(stepLevelPort, stepLevelData);

    return true;
}

// ============================================================================
// TimeDrivenRun — 逐点累积模式
// ============================================================================

bool AdaptLinQuant_Block::TimeDrivenRun()
{
    std::string inputPort    = GetInputPortName(0);
    std::string inStepPort   = GetInputPortName(1);
    std::string amplitudePort = GetOutputPortName(0);
    std::string outStepPort   = GetOutputPortName(1);
    std::string stepLevelPort = GetOutputPortName(2);

    auto inputData  = ReadInputData<double>(inputPort);
    auto inStepData = ReadInputData<double>(inStepPort);

    for (size_t i = 0; i < inputData.size(); ++i)
        m_inputBuffer.push_back(inputData[i]);
    for (size_t i = 0; i < inStepData.size(); ++i)
        m_inStepBuffer.push_back(inStepData[i]);

    if (static_cast<int>(m_inputBuffer.size()) >= 1
        && static_cast<int>(m_inStepBuffer.size()) >= 1)
    {
        const double step = m_inStepBuffer[0];

        if (!(step > 0.0) || !std::isfinite(step))
        {
            LOG_ERROR("AdaptLinQuant: inStep must be finite and > 0.");
            return false;
        }

        const unsigned int L_u = (1u << m_Bits);
        const double       L   = static_cast<double>(L_u);
        const double halfSpanMinusHalf = 0.5 * L - 0.5;

        const double x = m_inputBuffer[0];

        double kRound = std::floor(x / step + halfSpanMinusHalf + 0.5);

        if (kRound < 0.0)
            kRound = 0.0;

        const double kMax = static_cast<double>(L_u - 1u);
        if (kRound > kMax)
            kRound = kMax;

        const int k = static_cast<int>(kRound);

        const double q = (static_cast<double>(k) - halfSpanMinusHalf) * step;

        m_amplitudeQueue.push(q);
        m_outStepQueue.push(step);
        m_stepLevelQueue.push(k);

        m_inputBuffer.clear();
        m_inStepBuffer.clear();
    }

    if (!m_amplitudeQueue.empty())
    {
        std::vector<double> amplitudeData;
        amplitudeData.push_back(m_amplitudeQueue.front());
        m_amplitudeQueue.pop();
        WriteOutputData(amplitudePort, amplitudeData);
    }
    if (!m_outStepQueue.empty())
    {
        std::vector<double> outStepData;
        outStepData.push_back(m_outStepQueue.front());
        m_outStepQueue.pop();
        WriteOutputData(outStepPort, outStepData);
    }
    if (!m_stepLevelQueue.empty())
    {
        std::vector<int> stepLevelData;
        stepLevelData.push_back(m_stepLevelQueue.front());
        m_stepLevelQueue.pop();
        WriteOutputData(stepLevelPort, stepLevelData);
    }

    return true;
}

// ============================================================================
// Initialize
// ============================================================================

bool AdaptLinQuant_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_AdaptLinQuant = std::make_unique<AdaptLinQuant>();

    try
    {
        m_Bits = std::stoi(getParameter("Bits").Value);
    }
    catch (...) {}

    if (m_Bits < 1 || m_Bits > 31)
    {
        LOG_ERROR("AdaptLinQuant: Bits must be between 1 and 31.");
        return false;
    }

    AddInputPort("input",   m_AdaptLinQuant->input,  1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddInputPort("inStep",  m_AdaptLinQuant->inStep, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("amplitude", m_AdaptLinQuant->amplitude, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("outStep",   m_AdaptLinQuant->outStep,   1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("stepLevel", m_AdaptLinQuant->stepLevel, 1, Block::DataType::CIRCULAR_BUFFER_INT);

    return true;
}
