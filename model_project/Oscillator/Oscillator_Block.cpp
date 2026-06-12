#include "Oscillator_Block.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <complex>
#include <random>
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

Oscillator_Block::Oscillator_Block(const std::string& name)
    : Block(name)
{
}

void Oscillator_Block::SetDefaultParamters()
{
    m_frequency = 1e6;
    m_power = 0.010;
    m_phase = 0.0;
    m_randomPhase = Oscillator::No;
    m_ndensity = 0.0;
    m_refR = 50.0;
    m_showAdvancedParams = Oscillator::No;
    m_sampleRateOption = Oscillator::TimedFromSchematic;
    m_sampleRate = getSimu().samplingRate;
    m_initialDelay = 0.0;
}

void Oscillator_Block::SetParameters()
{
    if (!m_oscillator) {
        return;
    }

    m_oscillator->Frequency = m_frequency;
    m_oscillator->Power = m_power;
    m_oscillator->Phase = m_phase;
    m_oscillator->RandomPhase = m_randomPhase;
    m_oscillator->NDensity = m_ndensity;
    m_oscillator->RefR = m_refR;
    m_oscillator->ShowAdvancedParams = m_showAdvancedParams;
    m_oscillator->SampleRateOption = m_sampleRateOption;
    m_oscillator->SampleRate = m_sampleRate;
    m_oscillator->InitialDelay = m_initialDelay;
}

bool Oscillator_Block::Setup()
{
    Block::Setup();
    return true;
}

bool Oscillator_Block::Run()
{
    if (!CanProcess()) {
        return false;
    }

    if (!m_oscillator) {
        return false;
    }
    auto* outPort = GetOutputPort(GetOutputPortName(0));
    if (outPort && outPort->getCharacterizationFrequency() != m_frequency) {
        outPort->setCharacterizationFrequency(m_frequency);
    }
    const double t = simulator_param.startTime + static_cast<double>(m_oscillator->GetCount()) / m_sampleRate;

    const double StdDev = std::sqrt(m_ndensity * m_oscillator->output.GetSampleRate() * m_refR);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<double> dN(0, StdDev);

    SystemVueModelBuilder::EnvelopeSignal y;
    if (t < m_initialDelay) {
        y = 0.0;
    } else {
        const std::complex<double> tone = 10.0 * std::sqrt(m_power) * std::exp(std::complex<double>(0.0, m_phase));
        const std::complex<double> noise(dN(gen), dN(gen));
        y = tone + noise;
    }

    std::vector<SystemVueModelBuilder::EnvelopeSignal> outputData;
    outputData.push_back(y);
    WriteOutputData(GetOutputPortName(0), outputData);

    m_oscillator->Advance();

    return true;
}

bool Oscillator_Block::Initialize()
{
    SetBlockType(Block::BlockType::SOURCE);

    m_oscillator = std::make_unique<Oscillator>();

    AddOutputPort("output", m_oscillator->output, 1, Block::DataType::ENVELOPE_SIGNAL);

    SetDefaultParamters();

    simulator_param = getSimu();

    try { m_frequency = std::stod(getParameter("Frequency").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Frequency', using default value."); }
    try { m_power = std::stod(getParameter("Power").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Power', using default value."); }
    try { m_phase = std::stod(getParameter("Phase").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Phase', using default value."); }
    try { m_randomPhase = ConvertStringToYesOrNo(getParameter("RandomPhase").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'RandomPhase', using default value."); }
    try { m_ndensity = std::stod(getParameter("NDensity").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'NDensity', using default value."); }
    try { m_refR = std::stod(getParameter("RefR").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'RefR', using default value."); }
    try { m_showAdvancedParams = ConvertStringToYesOrNo(getParameter("ShowAdvancedParams").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'ShowAdvancedParams', using default value."); }
    try { m_sampleRateOption = ConvertStringToSampleRateOption(getParameter("SampleRateOption").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'SampleRateOption', using default value."); }
    try { m_sampleRate = std::stod(getParameter("SampleRate").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'SampleRate', using default value."); }
    try { m_initialDelay = std::stod(getParameter("InitialDelay").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'InitialDelay', using default value."); }

    SetParameters();

    if (m_frequency <= 0.0) {
        std::cout << "Oscillator: characterization frequency must be greater than 0." << std::endl;
        return false;
    }
    m_sampleRate = (m_sampleRateOption == Oscillator::TimedFromSampleRate)
        ? m_sampleRate
        : simulator_param.samplingRate;

    if (m_randomPhase == Oscillator::Yes) {
        const double pi = std::acos(-1.0);
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<double> dist(-pi, pi);
        m_phase = dist(gen);
        if (m_oscillator) {
            m_oscillator->Phase = m_phase;
        }
    }

    m_oscillator->output.SetSampleRate(m_sampleRate);

    if (!m_oscillator->Setup()) {
        return false;
    }

    return true;
}

Oscillator::SelectedYesOrNo Oscillator_Block::ConvertStringToYesOrNo(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "no") {
        return Oscillator::No;
    }
    if (lower == "yes" || lower == "1") {
        return Oscillator::Yes;
    }
    return Oscillator::No;
}

Oscillator::SelectedSampleRateOption Oscillator_Block::ConvertStringToSampleRateOption(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "untimed" || lower == "0") {
        return Oscillator::UnTimed;
    }
    if (lower == "timed from samplerate" || lower == "timedfromsamplerate" || lower == "1") {
        return Oscillator::TimedFromSampleRate;
    }
    if (lower == "timed from schematic" || lower == "timedfromschematic" || lower == "2") {
        return Oscillator::TimedFromSchematic;
    }
    return Oscillator::TimedFromSchematic;
}

