#include "LPF_ChebyshevII_Block.h"
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

LPF_ChebyshevII_Block::LPF_ChebyshevII_Block(const std::string& name)
    : Block(name)
{
}

void LPF_ChebyshevII_Block::SetDefaultParamters()
{
    m_loss = 0.0;
    m_passFreq = 100e3;
    m_passAtten = 3.0;
    m_stopFreq = 150e3;
    m_stopRipple = 50.0;
    m_orderType = LPF_ChebyshevII::UserDefined;
    m_order = 5;
    m_underSampledModel = LPF_ChebyshevII::ModelAsAllpass;
    m_sampleRate = getSimu().samplingRate;
}

void LPF_ChebyshevII_Block::SetParameters()
{
    if (!m_lpf) {
        return;
    }

    m_lpf->Loss = m_loss;
    m_lpf->PassFreq = m_passFreq;
    m_lpf->PassAtten = m_passAtten;
    m_lpf->StopFreq = m_stopFreq;
    m_lpf->StopRipple = m_stopRipple;
    m_lpf->OrderType = m_orderType;
    m_lpf->Order = m_order;
    m_lpf->UnderSampledModel = m_underSampledModel;
    m_lpf->SampleRate = m_sampleRate;
}

bool LPF_ChebyshevII_Block::Setup()
{
    Block::Setup();
    return true;
}

bool LPF_ChebyshevII_Block::Run()
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

    if(m_sampleRate > 2*m_passFreq) {
        for (size_t i = 0; i < inputData.size(); ++i) {
            const double t = simulator_param.startTime + static_cast<double>(m_lpf->GetCount()) / m_sampleRate;
            const std::complex<double> in = inputData[i].complex();
            const std::complex<double> expShift = m_lpf->complexExponential(fc, t);
            const std::complex<double> inExp = in * expShift;

            const double real = m_lpf->shelfFilterReal.filter(inExp.real());
            const double imag = m_lpf->shelfFilterImag.filter(inExp.imag());
            const std::complex<double> zz(real, imag);

            std::complex<double> out = zz * m_lpf->complexExponential(-fc, t);
            out /= m_lpf->dBToPowerRatio(m_loss / 2.0);

            outputData.emplace_back(out);
        }
    }
    else if(m_sampleRate <= 2*m_passFreq) {
        for (size_t i = 0; i < inputData.size(); ++i) {
            outputData.emplace_back(inputData[i]);
        }
    }


    WriteOutputData(outputPort, outputData);
    m_lpf->Advance();

    return true;
}

bool LPF_ChebyshevII_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_lpf = std::make_unique<LPF_ChebyshevII>();

    AddInputPort("input", m_lpf->input, 1, Block::DataType::ENVELOPE_SIGNAL);
    AddOutputPort("output", m_lpf->output, 1, Block::DataType::ENVELOPE_SIGNAL);

    SetDefaultParamters();
    simulator_param = getSimu();

    try { m_loss = std::stod(getParameter("Loss").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Loss', using default value."); }
    try { m_passFreq = std::stod(getParameter("PassFreq").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'PassFreq', using default value."); }
    try { m_passAtten = std::stod(getParameter("PassAtten").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'PassAtten', using default value."); }
    try { m_stopFreq = std::stod(getParameter("StopFreq").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'StopFreq', using default value."); }
    try { m_stopRipple = std::stod(getParameter("StopRipple").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'StopRipple', using default value."); }
    try { m_orderType = ConvertStringToOrderType(getParameter("OrderType").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'OrderType', using default value."); }
    try { m_order = std::stoi(getParameter("Order").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Order', using default value."); }
    try { m_underSampledModel = ConvertStringToUnderSampledModel(getParameter("UnderSampledModel").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'UnderSampledModel', using default value."); }
    try { m_sampleRate = std::stod(getParameter("SampleRate").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'SampleRate', using default value."); }

    if (m_sampleRate <= 0.0) {
        m_sampleRate = simulator_param.samplingRate;
    }
    SetParameters();

    if (!m_lpf->Setup()) {
        return false;
    }

    m_lpf->input.SetRate(1U);
    m_lpf->input.SetStartTime(simulator_param.startTime); // TODO: input not connected; timing setup may be unreliable
    m_lpf->output.SetStartTime(simulator_param.startTime);
    return true;
}

void LPF_ChebyshevII_Block::UpdateCharacterizationFrequency()
{
    const double fcIn = GetInputPort(GetInputPortName(0))->getCharacterizationFrequency();
    GetOutputPort(GetOutputPortName(0))->setCharacterizationFrequency(fcIn);
}

LPF_ChebyshevII::SelectedOrderType LPF_ChebyshevII_Block::ConvertStringToOrderType(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "userdefined") {
        return LPF_ChebyshevII::UserDefined;
    }
    if (lower == "auto" || lower == "0") {
        return LPF_ChebyshevII::Auto;
    }
    return LPF_ChebyshevII::UserDefined;
}

LPF_ChebyshevII::SelectedUnderSampledModel LPF_ChebyshevII_Block::ConvertStringToUnderSampledModel(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "modelasallpass") {
        return LPF_ChebyshevII::ModelAsAllpass;
    }
    if (lower == "error out" || lower == "errorout" || lower == "1") {
        return LPF_ChebyshevII::ErrorOut;
    }
    return LPF_ChebyshevII::ModelAsAllpass;
}




















