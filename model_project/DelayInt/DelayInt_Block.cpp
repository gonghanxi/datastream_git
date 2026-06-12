#include "DelayInt_Block.h"
#include <algorithm>

DelayInt_Block::DelayInt_Block(const std::string& name)
    : Block(name)
{
}

void DelayInt_Block::SetDefaultParamters()
{
    m_n = 1;
    m_outputTiming = DelayInt::EqualToInput;
    m_head = 0;
    m_warmup = 0;
}

void DelayInt_Block::SetParameters(int n, DelayInt::OutputTimingEnum timing)
{
    m_n = n;
    m_outputTiming = timing;
    if (m_delayInt) {
        m_delayInt->N = n;
        m_delayInt->OutputTiming = timing;
    }
}

void DelayInt_Block::ResetState()
{
    m_buf.clear();
    m_head = 0;
    m_warmup = 0;

    if (m_n > 0) {
        m_buf.assign(static_cast<std::size_t>(m_n), 0);
        if (m_outputTiming == DelayInt::BeforeInput) {
            m_warmup = m_n;
        }
    }
}

bool DelayInt_Block::Setup()
{
    Block::Setup();
    return true;
}

bool DelayInt_Block::Run()
{
    std::string inputPort = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    auto inputData = ReadInputData<int>(inputPort);
    if (inputData.empty()) {
        return true;
    }

    std::vector<int> outputData;
    outputData.reserve(inputData.size());

    if (m_n == 0) {
        for (size_t i = 0; i < inputData.size(); ++i) {
            outputData.push_back(inputData[i]);
        }
    } else {
        for (size_t i = 0; i < inputData.size(); ++i) {
            if (m_outputTiming == DelayInt::BeforeInput && m_warmup > 0) {
                outputData.push_back(0);
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

bool DelayInt_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_delayInt = std::make_unique<DelayInt>();

    m_delayInt->input.SetRate(1U);
    m_delayInt->output.SetRate(1U);

    AddInputPort("input", m_delayInt->input, 1, Block::DataType::CIRCULAR_BUFFER_INT);
    AddOutputPort("output", m_delayInt->output, 1, Block::DataType::CIRCULAR_BUFFER_INT);

    SetDefaultParamters();

    try { m_n = std::stoi(getParameter("N").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'N', using default value."); }
    try { m_outputTiming = ConvertStringToOutputTimingEnum(getParameter("OutputTiming").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'OutputTiming', using default value."); }

    SetParameters(m_n, m_outputTiming);
    ResetState();

    return true;
}

DelayInt::OutputTimingEnum DelayInt_Block::ConvertStringToOutputTimingEnum(const std::string& value)
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
        [](unsigned char c) { return std::tolower(c); });

    if (lowerValue == "equaltoinput" || lowerValue == "0") {
        return DelayInt::EqualToInput;
    } else if (lowerValue == "beforeinput" || lowerValue == "1") {
        return DelayInt::BeforeInput;
    }
    return DelayInt::EqualToInput;
}
