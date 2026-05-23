#include "Mixer_Block.h"

#include <algorithm>
#include <cctype>
#include <complex>
#include <cmath>

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

Mixer_Block::Mixer_Block(const std::string& name)
    : Block(name)
{
}

void Mixer_Block::SetDefaultParamters()
{
    m_convGain = 0.0;
    m_enableNoise = Mixer::YES;
    m_noiseFigure = 0.0;
    m_sideband = Mixer::Lower;
    m_sidebandSuppression = -200.0;
    m_rfRej = -200.0;
    m_loRej = -200.0;
    m_loRfIso = -200.0;
    m_rfLoIso = -200.0;
    m_soiOut = 1.0e17;
    m_toiOut = 1.0e17;
    m_refR = 50.0;
}

void Mixer_Block::SetParameters()
{
    if (!m_mixer) {
        return;
    }

    m_mixer->ConvGain = m_convGain;
    m_mixer->EnableNoise = m_enableNoise;
    m_mixer->NoiseFigure = m_noiseFigure;
    m_mixer->Sideband = m_sideband;
    m_mixer->SidebandSuppression = m_sidebandSuppression;
    m_mixer->RfRej = m_rfRej;
    m_mixer->LoRej = m_loRej;
    m_mixer->LoRfIso = m_loRfIso;
    m_mixer->RfLoIso = m_rfLoIso;
    m_mixer->SOIout = m_soiOut;
    m_mixer->TOIout = m_toiOut;
    m_mixer->RefR = m_refR;
}

bool Mixer_Block::Setup()
{
    Block::Setup();
    while(!m_outputQueue.empty()) m_outputQueue.pop();
    return true;
}

bool Mixer_Block::UpdateCharacterizationFrequency(double& fcOut)
{
    const double fcIn = GetInputPort(GetInputPortName(0))->getCharacterizationFrequency();
    const double fcLo = GetInputPort(GetInputPortName(1))->getCharacterizationFrequency();

    if (fcIn > 0.0 && fcLo > 0.0) {
        fcOut = (m_sideband == Mixer::Upper) ? (fcIn + fcLo) : std::abs(fcIn - fcLo);

        auto* outPort = GetOutputPort(GetOutputPortName(0));
        if (outPort && outPort->getCharacterizationFrequency() != fcOut) {
            outPort->setCharacterizationFrequency(fcOut);
        }
        return true;
    }


    std::cout << "characterization frequency must be greater than 0." << std::endl;
    return false;
}

bool Mixer_Block::DataStreamRun()
{
    if (!m_mixer) {
        return false;
    }

    const std::string inPortName = GetInputPortName(0);
    const std::string loPortName = GetInputPortName(1);
    const std::string outPortName = GetOutputPortName(0);

    auto inData = ReadInputData<SystemVueModelBuilder::EnvelopeSignal>(inPortName);
    if (inData.empty()) {
        return false;
    }

    auto loData = ReadInputData<SystemVueModelBuilder::EnvelopeSignal>(loPortName);
    if (loData.empty()) {
        return false;
    }

    std::vector<SystemVueModelBuilder::EnvelopeSignal> outputData;
    outputData.reserve(inData.size());

    const size_t loCount = loData.size();
    for (size_t i = 0; i < inData.size(); ++i) {
        const size_t li = (i < loCount) ? i : (loCount - 1);

        const std::complex<double> lo = loData[li].complex();
        const double magLo = std::sqrt(lo.real() * lo.real() + lo.imag() * lo.imag());

        if (magLo <= 0.0) {
            outputData.push_back(SystemVueModelBuilder::EnvelopeSignal(0.0));
            continue;
        }

        const std::complex<double> y = inData[i].complex() * lo / magLo;
        outputData.push_back(SystemVueModelBuilder::EnvelopeSignal(y));
    }

    WriteOutputData(outPortName, outputData);

    double fcOut = 0.0;
    if (!UpdateCharacterizationFrequency(fcOut)) {
        return false;
    }

    return true;
}

bool Mixer_Block::TimeDrivenRun()
{
    const std::string inPortName = GetInputPortName(0);
    const std::string loPortName = GetInputPortName(1);
    const std::string outPortName = GetOutputPortName(0);

    auto inData = ReadInputData<SystemVueModelBuilder::EnvelopeSignal>(inPortName);
    auto loData = ReadInputData<SystemVueModelBuilder::EnvelopeSignal>(loPortName);
    if (inData.empty() || loData.empty()) {
        return true;
    }
    m_inBuffer.push_back(inData[0]);
    m_loBuffer.push_back(loData[0]);
    if(m_inBuffer.size() >= 1 && m_loBuffer.size() >= 1) {
        std::vector<SystemVueModelBuilder::EnvelopeSignal> outputData;
        outputData.reserve(m_inBuffer.size());
        const size_t loCount = m_loBuffer.size();
        for (size_t i = 0; i < m_inBuffer.size(); ++i) {
            const size_t li = (i < loCount) ? i : (loCount - 1);

            const std::complex<double> lo = m_loBuffer[li].complex();
            const double magLo = std::sqrt(lo.real() * lo.real() + lo.imag() * lo.imag());

            if (magLo <= 0.0) {
                outputData.push_back(SystemVueModelBuilder::EnvelopeSignal(0.0));
                continue;
            }

            const std::complex<double> y = m_inBuffer[i].complex() * lo / magLo;
            outputData.push_back(SystemVueModelBuilder::EnvelopeSignal(y));
        }

        for(const auto& val : outputData) m_outputQueue.push(val);

        if (!m_outputQueue.empty())
        {
            EnvelopeSignal outputValue = m_outputQueue.front();
            m_outputQueue.pop();
            m_outputCount++;
            WriteOutputData(GetOutputPortName(0), std::vector<EnvelopeSignal>{outputValue});
            m_lastOutput = outputValue;
            m_inBuffer.clear();

            qDebug() << "[Mixer_Block] 分发输出:" << m_outputCount
                     << " value:" << outputValue.real() << "," << outputValue.imag();
        }

        double fcOut = 0.0;
        if (!UpdateCharacterizationFrequency(fcOut)) {
            return false;
        }
    }

    return true;
}

bool Mixer_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool Mixer_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_mixer = std::make_unique<Mixer>();

    AddInputPort("inPort", m_mixer->inPort, 1, Block::DataType::ENVELOPE_SIGNAL);
    AddInputPort("loPort", m_mixer->loPort, 1, Block::DataType::ENVELOPE_SIGNAL);
    AddOutputPort("outPort", m_mixer->outPort, 1, Block::DataType::ENVELOPE_SIGNAL);

    SetDefaultParamters();

    try { m_convGain = std::stod(getParameter("ConvGain").Value); } catch (...) { }
    try { m_enableNoise = ConvertStringToEnableNoise(getParameter("EnableNoise").Value); } catch (...) { }
    try { m_noiseFigure = std::stod(getParameter("NoiseFigure").Value); } catch (...) { }
    try { m_sideband = ConvertStringToSideband(getParameter("Sideband").Value); } catch (...) { }
    try { m_sidebandSuppression = std::stod(getParameter("SidebandSuppression").Value); } catch (...) { }
    try { m_rfRej = std::stod(getParameter("RfRej").Value); } catch (...) { }
    try { m_loRej = std::stod(getParameter("LoRej").Value); } catch (...) { }
    try { m_loRfIso = std::stod(getParameter("LoRfIso").Value); } catch (...) { }
    try { m_rfLoIso = std::stod(getParameter("RfLoIso").Value); } catch (...) { }
    try { m_soiOut = std::stod(getParameter("SOIout").Value); } catch (...) { }
    try { m_toiOut = std::stod(getParameter("TOIout").Value); } catch (...) { }
    try { m_refR = std::stod(getParameter("RefR").Value); } catch (...) { }

    SetParameters();
    return true;
}

Mixer::EnableNoiseEnum Mixer_Block::ConvertStringToEnableNoise(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "no" || lower == "0") {
        return Mixer::NO;
    }
    if (lower == "yes" || lower == "1") {
        return Mixer::YES;
    }
    return Mixer::YES;
}

Mixer::SidebandEnum Mixer_Block::ConvertStringToSideband(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "lower" || lower == "0") {
        return Mixer::Lower;
    }
    if (lower == "upper" || lower == "1") {
        return Mixer::Upper;
    }
    return Mixer::Lower;
}
