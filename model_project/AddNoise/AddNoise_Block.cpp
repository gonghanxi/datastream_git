#include "AddNoise_Block.h"

// ============================================================================
// 构造函数
// ============================================================================

AddNoise_Block::AddNoise_Block(const std::string& name)
    : Block(name)
{
}

// ============================================================================
// Setup
// ============================================================================

bool AddNoise_Block::Setup()
{
    Block::Setup();
    while (!m_outputQueue.empty()) m_outputQueue.pop();
    m_inputBuffer.clear();
    return true;
}

// ============================================================================
// Run — 分发
// ============================================================================

bool AddNoise_Block::Run()
{
    return DataStreamRun();
}

// ============================================================================
// DataStreamRun — 批量模式
// ============================================================================

bool AddNoise_Block::DataStreamRun()
{
    std::string inputPort  = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    auto inputData = ReadInputData<EnvelopeSignal>(inputPort);
    if (inputData.empty()) return true;

    const double k = 1.3806504e-23;
    const double NDensity = k * (m_SystemNoiseTemperature + 273.15) * std::pow(10.0, m_NoiseFigure / 10.0);
    const double StdDev   = std::sqrt(NDensity * m_Bandwidth * m_RefR);

    // 生成复数高斯噪声
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<double> dNRe(0, StdDev);
    std::normal_distribution<double> dNIm(0, StdDev);

    std::vector<EnvelopeSignal> outputData;
    for (size_t i = 0; i < inputData.size(); ++i)
    {
        std::complex<double> noise(dNRe(gen), dNIm(gen));
        outputData.push_back(EnvelopeSignal(inputData[i].complex() + noise));
    }

    WriteOutputData(outputPort, outputData);

    return true;
}

// ============================================================================
// Initialize
// ============================================================================

bool AddNoise_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_AddNoise = std::make_unique<AddNoise>();

    // 解析参数
    try { m_Bandwidth              = std::stod(getParameter("Bandwidth").Value);              } catch (...) {}
    try { m_NoiseFigure            = std::stod(getParameter("NoiseFigure").Value);            } catch (...) {}
    try { m_SystemNoiseTemperature = std::stod(getParameter("SystemNoiseTemperature").Value); } catch (...) {}
    try { m_RefR                   = std::stod(getParameter("RefR").Value);                   } catch (...) {}

    if (m_Bandwidth < 0)
    {
        LOG_ERROR("AddNoise: Bandwidth must be >= 0.");
        return false;
    }
    if (m_SystemNoiseTemperature < -273.15)
    {
        LOG_ERROR("AddNoise: SystemNoiseTemperature must be >= -273.15.");
        return false;
    }
    if (m_RefR <= 0)
    {
        LOG_ERROR("AddNoise: RefR must be > 0.");
        return false;
    }

    AddInputPort("input",  m_AddNoise->input,  1, Block::DataType::ENVELOPE_SIGNAL);
    AddOutputPort("output", m_AddNoise->output, 1, Block::DataType::ENVELOPE_SIGNAL);

    return true;
}
