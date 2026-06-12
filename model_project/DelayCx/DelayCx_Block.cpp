#include "DelayCx_Block.h"
#include <algorithm>
#include <cctype>

DelayCx_Block::DelayCx_Block(const std::string& name)
    : Block(name)
{
}

void DelayCx_Block::SetDefaultParamters()
{
    m_n = 1;
    m_outputTiming = DelayCx::EqualToInput;
    m_head = 0;
    m_warmup = 0;
}

void DelayCx_Block::SetParameters(int n, DelayCx::OutputTimingEnum timing)
{
    m_n = n;
    m_outputTiming = timing;
    if (m_delayCx) {
        m_delayCx->N = n;
        m_delayCx->OutputTiming = timing;
    }
}

void DelayCx_Block::ResetState()
{
    m_buf.clear();
    m_head = 0;
    m_warmup = 0;

    if (m_n > 0) {
        m_buf.assign(static_cast<std::size_t>(m_n), std::complex<double>(0.0, 0.0));
        if (m_outputTiming == DelayCx::BeforeInput) {
            m_warmup = m_n;
        }
    }
}

bool DelayCx_Block::Setup()
{
    Block::Setup();
    return true;
}

bool DelayCx_Block::Run()
{
    std::string inputPort = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    auto inputData = ReadInputData<std::complex<double>>(inputPort);
    if (inputData.empty()) {
        return true;
    }

    std::vector<std::complex<double>> outputData;
    outputData.reserve(inputData.size());

    if (m_n == 0) {
        for (size_t i = 0; i < inputData.size(); ++i) {
            outputData.push_back(inputData[i]);
        }
    } else {
        for (size_t i = 0; i < inputData.size(); ++i) {
            if (m_outputTiming == DelayCx::BeforeInput && m_warmup > 0) {
                outputData.push_back(std::complex<double>(0.0, 0.0));
                if (!m_buf.empty()) {
                    m_buf[m_head] = inputData[i];
                    m_head = (m_head + 1) % m_buf.size();
                }
                --m_warmup;
                continue;
            }
            outputData.push_back(m_buf[m_head]);
            m_buf[m_head] = inputData[i];
            m_head = (m_head + 1) % m_buf.size();
        }
    }

    WriteOutputData(outputPort, outputData);

    return true;
}

bool DelayCx_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_delayCx = std::make_unique<DelayCx>();

    m_delayCx->input.SetRate(1U);
    m_delayCx->output.SetRate(1U);

    AddInputPort("input", m_delayCx->input, 1, Block::DataType::CIRCULAR_BUFFER_DCOMPLEX);
    AddOutputPort("output", m_delayCx->output, 1, Block::DataType::CIRCULAR_BUFFER_DCOMPLEX);

    SetDefaultParamters();

    try { m_n = std::stoi(getParameter("N").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'N', using default value."); }
    try { m_outputTiming = ConvertStringToOutputTimingEnum(getParameter("OutputTiming").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'OutputTiming', using default value."); }

    SetParameters(m_n, m_outputTiming);
    ResetState();

    return true;
}

DelayCx::OutputTimingEnum DelayCx_Block::ConvertStringToOutputTimingEnum(const std::string& value)
{
    std::string trimmedValue;
    trimmedValue.reserve(value.size());
    for (char c : value) {
        if (!std::isspace(static_cast<unsigned char>(c))) {
            trimmedValue.push_back(c);
        }
    }

    std::string lowerValue = trimmedValue;
    std::transform(lowerValue.begin(), lowerValue.end(), lowerValue.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    if (lowerValue == "equaltoinput" || lowerValue == "0") {
        return DelayCx::EqualToInput;
    } else if (lowerValue == "beforeinput" || lowerValue == "1") {
        return DelayCx::BeforeInput;
    }
    return DelayCx::EqualToInput;
}
