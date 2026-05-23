#include "BSF_ChebyshevI_Block.h"
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

BSF_ChebyshevI_Block::BSF_ChebyshevI_Block(const std::string& name)
    : Block(name)
{
}

void BSF_ChebyshevI_Block::SetDefaultParamters()
{
    m_loss = 0.0;
    m_fCenter = 150e3;
    m_passBandwidth = 100e3;
    m_passRipple = 1.0;
    m_stopBandwidth = 50e3;
    m_stopAtten = 50.0;
    m_orderType = BSF_ChebyshevI::UserDefined;
    m_order = 5;
    m_sampleRate = getSimu().samplingRate;
}

void BSF_ChebyshevI_Block::SetParameters()
{
    if (!m_bsf) {
        return;
    }

    m_bsf->Loss = m_loss;
    m_bsf->FCenter = m_fCenter;
    m_bsf->PassBandwidth = m_passBandwidth;
    m_bsf->PassRipple = m_passRipple;
    m_bsf->StopBandwidth = m_stopBandwidth;
    m_bsf->StopAtten = m_stopAtten;
    m_bsf->OrderType = m_orderType;
    m_bsf->Order = m_order;
    m_bsf->SampleRate = m_sampleRate;
}

bool BSF_ChebyshevI_Block::Setup()
{
    Block::Setup();
    return true;
}

bool BSF_ChebyshevI_Block::Run()
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

    for (size_t i = 0; i < inputData.size(); ++i) {
        const double t = simulator_param.startTime + static_cast<double>(m_bsf->GetCount()) / m_sampleRate;
        const std::complex<double> in = inputData[i].complex();
        const std::complex<double> expShift = m_bsf->complexExponential(fc - m_fCenter, t);
        const std::complex<double> inExp = in * expShift;

        const double real = m_bsf->shelfFilterReal.filter(inExp.real());
        const double imag = m_bsf->shelfFilterImag.filter(inExp.imag());
        const std::complex<double> zz(real, imag);

        std::complex<double> out = zz * m_bsf->complexExponential(m_fCenter - fc, t);
        out /= m_bsf->dBToPowerRatio(m_loss / 2.0);

        outputData.emplace_back(out);
    }

    WriteOutputData(outputPort, outputData);
    m_bsf->Advance();

    return true;
}

bool BSF_ChebyshevI_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_bsf = std::make_unique<BSF_ChebyshevI>();

    AddInputPort("input", m_bsf->input, 1, Block::DataType::ENVELOPE_SIGNAL);
    AddOutputPort("output", m_bsf->output, 1, Block::DataType::ENVELOPE_SIGNAL);

    SetDefaultParamters();
    simulator_param = getSimu();

    try { m_loss = std::stod(getParameter("Loss").Value); } catch (...) { }
    try { m_fCenter = std::stod(getParameter("FCenter").Value); } catch (...) { }
    try { m_passBandwidth = std::stod(getParameter("PassBandwidth").Value); } catch (...) { }
    try { m_passRipple = std::stod(getParameter("PassRipple").Value); } catch (...) { }
    try { m_stopBandwidth = std::stod(getParameter("StopBandwidth").Value); } catch (...) { }
    try { m_stopAtten = std::stod(getParameter("StopAtten").Value); } catch (...) { }
    try { m_orderType = ConvertStringToOrderType(getParameter("OrderType").Value); } catch (...) { }
    try { m_order = std::stoi(getParameter("Order").Value); } catch (...) { }
    try { m_sampleRate = std::stod(getParameter("SampleRate").Value); } catch (...) { }

    if (m_sampleRate <= 0.0) {
        m_sampleRate = simulator_param.samplingRate;
    }
    SetParameters();

    if (!m_bsf->Setup()) {
        return false;
    }

    m_bsf->input.SetRate(1U);
    m_bsf->input.SetStartTime(simulator_param.startTime); // TODO: input not connected; timing setup may be unreliable
    m_bsf->output.SetStartTime(simulator_param.startTime);
    return true;
}

void BSF_ChebyshevI_Block::UpdateCharacterizationFrequency()
{
    const double fcIn = GetInputPort(GetInputPortName(0))->getCharacterizationFrequency();
    GetOutputPort(GetOutputPortName(0))->setCharacterizationFrequency(fcIn);
}

BSF_ChebyshevI::SelectedOrderType BSF_ChebyshevI_Block::ConvertStringToOrderType(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "userdefined") {
        return BSF_ChebyshevI::UserDefined;
    }
    if (lower == "auto" || lower == "0") {
        return BSF_ChebyshevI::Auto;
    }
    return BSF_ChebyshevI::UserDefined;
}




















