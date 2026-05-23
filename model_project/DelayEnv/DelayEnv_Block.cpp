#include "DelayEnv_Block.h"
#include <algorithm>
#include <cctype>
#include <iostream>

DelayEnv_Block::DelayEnv_Block(const std::string& name)
    : Block(name)
{
}

void DelayEnv_Block::SetDefaultParamters()
{
    m_n = 1;
    m_outputTiming = DelayEnv::EqualToInput;
    m_head = 0;
    m_warmup = 0;
}

void DelayEnv_Block::SetParameters(int n, DelayEnv::OutputTimingEnum timing)
{
    m_n = n;
    m_outputTiming = timing;
    if (m_delayEnv) {
        m_delayEnv->N = n;
        m_delayEnv->OutputTiming = timing;
    }
}

void DelayEnv_Block::ResetState()
{
    m_buf.clear();
    m_head = 0;
    m_warmup = 0;

    if (m_n > 0) {
        m_buf.assign(static_cast<std::size_t>(m_n), SystemVueModelBuilder::EnvelopeSignal(0.0));
        if (m_outputTiming == DelayEnv::BeforeInput) {
            m_warmup = m_n;
        }
    }
}

bool DelayEnv_Block::Setup()
{
    Block::Setup();
    return true;
}

bool DelayEnv_Block::Run()
{
    std::string inputPort = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    auto inputData = ReadInputData<SystemVueModelBuilder::EnvelopeSignal>(inputPort);
    if (inputData.empty()) {
        return true;
    }

    UpdateCharacterizationFrequency();

    std::vector<SystemVueModelBuilder::EnvelopeSignal> outputData;
    outputData.reserve(inputData.size());

    if (m_n == 0) {
        for (size_t i = 0; i < inputData.size(); ++i) {
            outputData.push_back(inputData[i]);
        }
    } else {
        for (size_t i = 0; i < inputData.size(); ++i) {
            if (m_outputTiming == DelayEnv::BeforeInput && m_warmup > 0) {
                outputData.push_back(SystemVueModelBuilder::EnvelopeSignal(0.0));
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

bool DelayEnv_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_delayEnv = std::make_unique<DelayEnv>();

    m_delayEnv->input.SetRate(1U);
    m_delayEnv->output.SetRate(1U);

    AddInputPort("input", m_delayEnv->input, 1, Block::DataType::ENVELOPE_SIGNAL);
    AddOutputPort("output", m_delayEnv->output, 1, Block::DataType::ENVELOPE_SIGNAL);

    SetDefaultParamters();

    try { m_n = std::stoi(getParameter("N").Value); } catch (...) { }
    try { m_outputTiming = ConvertStringToOutputTimingEnum(getParameter("OutputTiming").Value); } catch (...) { }

    if (m_n < 0) {
        std::cout << "DelayEnv: N must be >= 0." << std::endl;
        return false;
    }

    SetParameters(m_n, m_outputTiming);
    ResetState();

    return true;
}

void DelayEnv_Block::UpdateCharacterizationFrequency()
{
    auto* inPort = GetInputPort(GetInputPortName(0));
    auto* outPort = GetOutputPort(GetOutputPortName(0));
    if (!inPort || !outPort) {
        return;
    }

    const double fcIn = inPort->getCharacterizationFrequency();
    outPort->setCharacterizationFrequency(fcIn);
}

DelayEnv::OutputTimingEnum DelayEnv_Block::ConvertStringToOutputTimingEnum(const std::string& value)
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
        return DelayEnv::EqualToInput;
    } else if (lowerValue == "beforeinput" || lowerValue == "1") {
        return DelayEnv::BeforeInput;
    }
    return DelayEnv::EqualToInput;
}


