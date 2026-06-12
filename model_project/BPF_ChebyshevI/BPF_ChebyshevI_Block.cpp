#include "BPF_ChebyshevI_Block.h"
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

BPF_ChebyshevI_Block::BPF_ChebyshevI_Block(const std::string& name)
    : Block(name)
{
}

void BPF_ChebyshevI_Block::SetDefaultParamters()
{
    m_loss = 0.0;
    m_fCenter = 150e3;
    m_passBandwidth = 50e3;
    m_passRipple = 1.0;
    m_stopBandwidth = 100e3;
    m_stopAtten = 50.0;
    m_orderType = BPF_ChebyshevI::UserDefined;
    m_order = 5;
    m_transform = BPF_ChebyshevI::Bilinear;
    m_underSampledModel = BPF_ChebyshevI::ModelAsAllpass;
    m_sampleRate = getSimu().samplingRate;
}

void BPF_ChebyshevI_Block::SetParameters()
{
    if (!m_bpf) {
        return;
    }

    m_bpf->Loss = m_loss;
    m_bpf->FCenter = m_fCenter;
    m_bpf->PassBandwidth = m_passBandwidth;
    m_bpf->PassRipple = m_passRipple;
    m_bpf->StopBandwidth = m_stopBandwidth;
    m_bpf->StopAtten = m_stopAtten;
    m_bpf->OrderType = m_orderType;
    m_bpf->Order = m_order;
    m_bpf->Transform = m_transform;
    m_bpf->UnderSampledModel = m_underSampledModel;
    m_bpf->SampleRate = m_sampleRate;
}

bool BPF_ChebyshevI_Block::Setup()
{
    Block::Setup();
    return true;
}

bool BPF_ChebyshevI_Block::Run()
{
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

    if(m_sampleRate > m_passBandwidth) {
        for (size_t i = 0; i < inputData.size(); ++i) {
            const double t = simulator_param.startTime + static_cast<double>(m_bpf->GetCount()) / m_sampleRate;
            const std::complex<double> in = inputData[i].complex();
            const std::complex<double> expShift = m_bpf->complexExponential(fc - m_fCenter, t);
            const std::complex<double> inExp = in * expShift;

            const double real = m_bpf->shelfFilterReal.filter(inExp.real());
            const double imag = m_bpf->shelfFilterImag.filter(inExp.imag());
            const std::complex<double> zz(real, imag);

            std::complex<double> out = zz * m_bpf->complexExponential(m_fCenter - fc, t);
            out /= m_bpf->dBToPowerRatio(m_loss / 2.0);

            outputData.emplace_back(out);
        }
    }
    else if(m_sampleRate <= m_passBandwidth) {
        for (size_t i = 0; i < inputData.size(); ++i) {
            outputData.emplace_back(inputData[i]);
        }
    }


    WriteOutputData(outputPort, outputData);
    m_bpf->Advance();

    return true;
}

bool BPF_ChebyshevI_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_bpf = std::make_unique<BPF_ChebyshevI>();

    AddInputPort("input", m_bpf->input, 1, Block::DataType::ENVELOPE_SIGNAL);
    AddOutputPort("output", m_bpf->output, 1, Block::DataType::ENVELOPE_SIGNAL);

    SetDefaultParamters();
    simulator_param = getSimu();

    try { m_loss = std::stod(getParameter("Loss").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Loss', using default value."); }
    try { m_fCenter = std::stod(getParameter("FCenter").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'FCenter', using default value."); }
    try { m_passBandwidth = std::stod(getParameter("PassBandwidth").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'PassBandwidth', using default value."); }
    try { m_passRipple = std::stod(getParameter("PassRipple").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'PassRipple', using default value."); }
    try { m_stopBandwidth = std::stod(getParameter("StopBandwidth").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'StopBandwidth', using default value."); }
    try { m_stopAtten = std::stod(getParameter("StopAtten").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'StopAtten', using default value."); }
    try { m_orderType = ConvertStringToOrderType(getParameter("OrderType").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'OrderType', using default value."); }
    try { m_order = std::stoi(getParameter("Order").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Order', using default value."); }
    try { m_transform = ConvertStringToTransform(getParameter("Transform").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Transform', using default value."); }
    try { m_underSampledModel = ConvertStringToUnderSampledModel(getParameter("UnderSampledModel").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'UnderSampledModel', using default value."); }
    try { m_sampleRate = std::stod(getParameter("SampleRate").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'SampleRate', using default value."); }

    if (m_sampleRate <= 0.0) {
        m_sampleRate = simulator_param.samplingRate;
    }
    SetParameters();

    if (!m_bpf->Setup()) {
        return false;
    }

    m_bpf->input.SetRate(1U);
    m_bpf->input.SetStartTime(simulator_param.startTime); // TODO: input not connected; timing setup may be unreliable
    m_bpf->output.SetStartTime(simulator_param.startTime);
    return true;
}

void BPF_ChebyshevI_Block::UpdateCharacterizationFrequency()
{
    const double fcIn = GetInputPort(GetInputPortName(0))->getCharacterizationFrequency();
    GetOutputPort(GetOutputPortName(0))->setCharacterizationFrequency(fcIn);
}

BPF_ChebyshevI::SelectedOrderType BPF_ChebyshevI_Block::ConvertStringToOrderType(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "userdefined") {
        return BPF_ChebyshevI::UserDefined;
    }
    if (lower == "auto" || lower == "0") {
        return BPF_ChebyshevI::Auto;
    }
    return BPF_ChebyshevI::UserDefined;
}

BPF_ChebyshevI::SelectedTransform BPF_ChebyshevI_Block::ConvertStringToTransform(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "bilinear") {
        return BPF_ChebyshevI::Bilinear;
    }
    if (lower == "impulse invariance" || lower == "impulseinvariance" || lower == "1") {
        return BPF_ChebyshevI::ImpulseInvariance;
    }
    return BPF_ChebyshevI::Bilinear;
}

BPF_ChebyshevI::SelectedUnderSampledModel BPF_ChebyshevI_Block::ConvertStringToUnderSampledModel(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "modelasallpass") {
        return BPF_ChebyshevI::ModelAsAllpass;
    }
    if (lower == "error out" || lower == "errorout" || lower == "1") {
        return BPF_ChebyshevI::ErrorOut;
    }
    return BPF_ChebyshevI::ModelAsAllpass;
}




















