#include "RampGen_Block.h"
#include <algorithm>
#include <cctype>
#include <cmath>
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

RampGen_Block::RampGen_Block(const std::string& name)
    : Block(name)
{
}

void RampGen_Block::SetDefaultParamters()
{
    m_loLevel = 0.0;
    m_hiLevel = 1.0;
    m_frequency = 5e3;
    m_phase = 0.0;
    m_symmetry = 100.0;
    m_polarity = RampGen::normal;
    m_showAdvancedParams = RampGen::No;
    m_sampleRateOption = RampGen::TimedFromSchematic;
    m_sampleRate = getSimu().samplingRate;
    m_initialDelay = 0.0;
    m_burstMode = RampGen::OFF;
    m_burstLength = 100e-6;
    m_burstPeriod = 200e-6;
    m_burstDelay = 0.0;
}

void RampGen_Block::SetParameters()
{
    if (!m_rampGen) {
        return;
    }

    m_rampGen->LoLevel = m_loLevel;
    m_rampGen->HiLevel = m_hiLevel;
    m_rampGen->Frequency = m_frequency;
    m_rampGen->Phase = m_phase;
    m_rampGen->Symmetry = m_symmetry;
    m_rampGen->Polarity = m_polarity;
    m_rampGen->ShowAdvancedParams = m_showAdvancedParams;
    m_rampGen->SampleRateOption = m_sampleRateOption;
    m_rampGen->SampleRate = m_sampleRate;
    m_rampGen->InitialDelay = m_initialDelay;
    m_rampGen->BurstMode = m_burstMode;
    m_rampGen->BurstLength = m_burstLength;
    m_rampGen->BurstPeriod = m_burstPeriod;
    m_rampGen->BurstDelay = m_burstDelay;
}

bool RampGen_Block::Setup()
{
    Block::Setup();
    return true;
}

bool RampGen_Block::Run()
{
    if (!CanProcess()) {
        return false;
    }

    const double sampleRate = (m_sampleRateOption == RampGen::TimedFromSampleRate)
        ? m_sampleRate
        : simulator_param.samplingRate;
    
    if (sampleRate <= 0.0) {
        return false;
    }

    if (m_frequency < 0.0 || m_frequency > sampleRate / 4.0) {
        return false;
    }

    if (m_burstLength < 1.0 / sampleRate) {
        return false;
    }

    if (m_burstPeriod < 1.0 / sampleRate) {
        return false;
    }

    const double PI = std::acos(-1.0);
    const double t = simulator_param.startTime
        + static_cast<double>(m_rampGen->GetCount()) / sampleRate
        + 1e-16;
    const double tInflection = m_symmetry / 100.0 / m_frequency;
    const double phaseD = std::fmod((std::fmod(m_phase, 2.0 * PI) + 2.0 * PI), 2.0 * PI);
    const double pt = std::fmod((t - m_initialDelay + phaseD / (2.0 * PI) / m_frequency), 1.0 / m_frequency);

    double y = 0.0;
    switch (m_burstMode) {
    case RampGen::OFF:
        if (pt < tInflection) {
            y = (m_polarity == RampGen::inverted)
                ? (m_hiLevel + (m_loLevel - m_hiLevel) * pt / tInflection)
                : (m_loLevel + (m_hiLevel - m_loLevel) * pt / tInflection);
        } else {
            y = (m_polarity == RampGen::inverted)
                ? (m_loLevel + (m_hiLevel - m_loLevel) * (pt - tInflection) / (1.0 / m_frequency - tInflection))
                : (m_hiLevel + (m_loLevel - m_hiLevel) * (pt - tInflection) / (1.0 / m_frequency - tInflection));
        }
        break;
    case RampGen::Single:
        if (t >= m_initialDelay && t < m_initialDelay + m_burstDelay) {
            y = (m_hiLevel + m_loLevel) / 2.0;
        } else if (t >= m_initialDelay + m_burstDelay && t < m_initialDelay + m_burstDelay + m_burstLength) {
            if (pt - m_burstDelay < tInflection) {
                y = (m_polarity == RampGen::inverted)
                    ? (m_hiLevel + (m_loLevel - m_hiLevel) * (pt - m_burstDelay) / tInflection)
                    : (m_loLevel + (m_hiLevel - m_loLevel) * (pt - m_burstDelay) / tInflection);
            } else {
                y = (m_polarity == RampGen::inverted)
                    ? (m_loLevel + (m_hiLevel - m_loLevel) * (pt - m_burstDelay - tInflection) / (1.0 / m_frequency - tInflection))
                    : (m_hiLevel + (m_loLevel - m_hiLevel) * (pt - m_burstDelay - tInflection) / (1.0 / m_frequency - tInflection));
            }
        } else {
            y = (m_hiLevel + m_loLevel) / 2.0;
        }
        break;
    case RampGen::Multiple: {
        const double wt = std::fmod(t - m_initialDelay - m_burstDelay, m_burstPeriod);
        if (t >= m_initialDelay && t < m_initialDelay + m_burstDelay) {
            y = (m_hiLevel + m_loLevel) / 2.0;
        } else if (wt <= m_burstLength) {
            if (pt - m_burstDelay < tInflection) {
                y = (m_polarity == RampGen::inverted)
                    ? (m_hiLevel + (m_loLevel - m_hiLevel) * (pt - m_burstDelay) / tInflection)
                    : (m_loLevel + (m_hiLevel - m_loLevel) * (pt - m_burstDelay) / tInflection);
            } else {
                y = (m_polarity == RampGen::inverted)
                    ? (m_loLevel + (m_hiLevel - m_loLevel) * (pt - m_burstDelay - tInflection) / (1.0 / m_frequency - tInflection))
                    : (m_hiLevel + (m_loLevel - m_hiLevel) * (pt - m_burstDelay - tInflection) / (1.0 / m_frequency - tInflection));
            }
        } else {
            y = (m_hiLevel + m_loLevel) / 2.0;
        }
        break;
    }
    default:
        break;
    }

    if (t < m_initialDelay) {
        y = 0.0;
    }

    std::vector<double> outputData;
    outputData.push_back(y);
    WriteOutputData(GetOutputPortName(0), outputData);

    m_rampGen->Advance();
    return true;
}

bool RampGen_Block::Initialize()
{
    SetBlockType(Block::BlockType::SOURCE);

    m_rampGen = std::make_unique<RampGen>();

    AddOutputPort("output", m_rampGen->output, 1, Block::DataType::TIMED_DOUBLE);

    SetDefaultParamters();
    simulator_param = getSimu();

    try { m_loLevel = std::stod(getParameter("LoLevel").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'LoLevel', using default value."); }
    try { m_hiLevel = std::stod(getParameter("HiLevel").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'HiLevel', using default value."); }
    try { m_frequency = std::stod(getParameter("Frequency").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Frequency', using default value."); }
    try { m_phase = std::stod(getParameter("Phase").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Phase', using default value."); }
    try { m_symmetry = std::stod(getParameter("Symmetry").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Symmetry', using default value."); }
    try { m_polarity = ConvertStringToPolarity(getParameter("Polarity").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Polarity', using default value."); }
    try { m_showAdvancedParams = ConvertStringToShowAdvancedParams(getParameter("ShowAdvancedParams").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'ShowAdvancedParams', using default value."); }
    try { m_sampleRateOption = ConvertStringToSampleRateOption(getParameter("SampleRateOption").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'SampleRateOption', using default value."); }
    try { m_sampleRate = std::stod(getParameter("SampleRate").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'SampleRate', using default value."); }
    
    if (m_sampleRate <= 0.0) {
        std::cout << "SampleRate must be greater than 0." << std::endl;
        return false;
    }
    try { m_initialDelay = std::stod(getParameter("InitialDelay").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'InitialDelay', using default value."); }
    try { m_burstMode = ConvertStringToBurstMode(getParameter("BurstMode").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'BurstMode', using default value."); }
    try { m_burstLength = std::stod(getParameter("BurstLength").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'BurstLength', using default value."); }
    try { m_burstPeriod = std::stod(getParameter("BurstPeriod").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'BurstPeriod', using default value."); }
    try { m_burstDelay = std::stod(getParameter("BurstDelay").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'BurstDelay', using default value."); }

    SetParameters();

    const double outFs = (m_sampleRateOption == RampGen::TimedFromSampleRate) ? m_sampleRate : getSimu().samplingRate;
    m_rampGen->output.SetSampleRate(outFs);

    if (!m_rampGen->Setup()) {
        return false;
    }

    return true;
}

RampGen::SelectedPolarity RampGen_Block::ConvertStringToPolarity(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "normal") {
        return RampGen::normal;
    }
    if (lower == "inverted" || lower == "1") {
        return RampGen::inverted;
    }
    return RampGen::normal;
}

RampGen::SelectedShowAdvancedParams RampGen_Block::ConvertStringToShowAdvancedParams(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "no") {
        return RampGen::No;
    }
    if (lower == "yes" || lower == "1") {
        return RampGen::Yes;
    }
    return RampGen::No;
}

RampGen::SelectedSampleRateOption RampGen_Block::ConvertStringToSampleRateOption(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "untimed" || lower == "0") {
        return RampGen::UnTimed;
    }
    if (lower == "timed from samplerate" || lower == "timedfromsamplerate" || lower == "1") {
        return RampGen::TimedFromSampleRate;
    }
    if (lower == "timed from schematic" || lower == "timedfromschematic" || lower == "2") {
        return RampGen::TimedFromSchematic;
    }
    return RampGen::TimedFromSchematic;
}

RampGen::SelectedBurstMode RampGen_Block::ConvertStringToBurstMode(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "off") {
        return RampGen::OFF;
    }
    if (lower == "single" || lower == "1") {
        return RampGen::Single;
    }
    if (lower == "multiple" || lower == "2") {
        return RampGen::Multiple;
    }
    return RampGen::OFF;
}








