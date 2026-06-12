#include "HPF_ChebyshevI_Block.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <complex>
#include <vector>

namespace {
std::string TrimCopy(const std::string& value)
{
    std::string s = value;
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
    return s;
}

std::string ToLowerCopy(const std::string& value)
{
    std::string s = value;
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return s;
}
}

HPF_ChebyshevI_Block::HPF_ChebyshevI_Block(const std::string& name)
    : Block(name)
{
}

void HPF_ChebyshevI_Block::SetDefaultParamters()
{
    m_loss = 0.0;
    m_passFreq = 150e3;
    m_passRipple = 1.0;
    m_stopFreq = 100e3;
    m_stopAtten = 50.0;
    m_orderType = HPF_ChebyshevI::UserDefined;
    m_order = 5;
    m_sampleRate = getSimu().samplingRate;
}

void HPF_ChebyshevI_Block::SetParameters()
{
    if (!m_hpf) {
        return;
    }

    m_hpf->Loss = m_loss;
    m_hpf->PassFreq = m_passFreq;
    m_hpf->PassRipple = m_passRipple;
    m_hpf->StopFreq = m_stopFreq;
    m_hpf->StopAtten = m_stopAtten;
    m_hpf->OrderType = m_orderType;
    m_hpf->Order = m_order;
    m_hpf->SampleRate = m_sampleRate;
}

bool HPF_ChebyshevI_Block::Setup()
{
    Block::Setup();
    return true;
}

bool HPF_ChebyshevI_Block::Run()
{

    if (!CanProcess()) {
        return false;
    }

    const std::string inputPort = GetInputPortName(0);
    const std::string outputPort = GetOutputPortName(0);

    auto inputData = ReadInputData<SystemVueModelBuilder::EnvelopeSignal>(inputPort);
    if (inputData.empty()) {
    return true;
    }
    UpdateCharacterizationFrequency();
const double fc = GetOutputPort(outputPort)->getCharacterizationFrequency();

    std::vector<SystemVueModelBuilder::EnvelopeSignal> outputData;
    outputData.reserve(inputData.size());

    for (size_t i = 0; i < inputData.size(); ++i) {
        const double t = simulator_param.startTime + static_cast<double>(m_hpf->GetCount()) / m_sampleRate;
        const std::complex<double> in = inputData[i].complex();
        const std::complex<double> expShift = m_hpf->complexExponential(fc, t);
        const std::complex<double> inExp = in * expShift;

        const double real = m_hpf->shelfFilterReal.filter(inExp.real());
        const double imag = m_hpf->shelfFilterImag.filter(inExp.imag());
        const std::complex<double> zz(real, imag);

        std::complex<double> out = zz * m_hpf->complexExponential(-fc, t);
        out /= m_hpf->dBToPowerRatio(m_loss / 2.0);

        outputData.emplace_back(out);
    }

    WriteOutputData(outputPort, outputData);
    m_hpf->Advance();

    return true;
}

bool HPF_ChebyshevI_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_hpf = std::make_unique<HPF_ChebyshevI>();

    AddInputPort("input", m_hpf->input, 1, Block::DataType::ENVELOPE_SIGNAL);
    AddOutputPort("output", m_hpf->output, 1, Block::DataType::ENVELOPE_SIGNAL);

    SetDefaultParamters();
    simulator_param = getSimu();

    try { m_loss = std::stod(getParameter("Loss").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Loss', using default value."); }
    try { m_passFreq = std::stod(getParameter("PassFreq").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'PassFreq', using default value."); }
    try { m_passRipple = std::stod(getParameter("PassRipple").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'PassRipple', using default value."); }
    try { m_stopFreq = std::stod(getParameter("StopFreq").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'StopFreq', using default value."); }
    try { m_stopAtten = std::stod(getParameter("StopAtten").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'StopAtten', using default value."); }
    try { m_orderType = ConvertStringToOrderType(getParameter("OrderType").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'OrderType', using default value."); }
    try { m_order = std::stoi(getParameter("Order").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Order', using default value."); }
    try { m_sampleRate = std::stod(getParameter("SampleRate").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'SampleRate', using default value."); }

    if (m_sampleRate <= 0.0) {
        m_sampleRate = simulator_param.samplingRate;
    }
    SetParameters();

    if (!m_hpf->Setup()) {
        return false;
    }

    m_hpf->input.SetRate(1U);
    m_hpf->input.SetStartTime(simulator_param.startTime); // TODO: input not connected; timing setup may be unreliable
    m_hpf->output.SetStartTime(simulator_param.startTime);
    return true;
}

void HPF_ChebyshevI_Block::UpdateCharacterizationFrequency()
{
    const double fcIn = GetInputPort(GetInputPortName(0))->getCharacterizationFrequency();
    GetOutputPort(GetOutputPortName(0))->setCharacterizationFrequency(fcIn);
}

HPF_ChebyshevI::SelectedOrderType HPF_ChebyshevI_Block::ConvertStringToOrderType(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "userdefined") {
        return HPF_ChebyshevI::UserDefined;
    }
    if (lower == "auto" || lower == "0") {
        return HPF_ChebyshevI::Auto;
    }
    return HPF_ChebyshevI::UserDefined;
}




















