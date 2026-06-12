#include "SineGen_Block.h"
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

SineGen_Block::SineGen_Block(const std::string& name)
    : Block(name)
{
}

void SineGen_Block::SetDefaultParamters()
{
    m_amplitude = 1.0;
    m_offset = 0.0;
    m_frequency = 5e3;
    m_phase = 0.0;
    m_showAdvancedParams = SineGen::No;
    m_sampleRateOption = SineGen::TimedFromSchematic;
    m_sampleRate = getSimu().samplingRate;
    m_initialDelay = 0.0;
    m_burstMode = SineGen::OFF;
    m_burstLength = 100e-6;
    m_burstPeriod = 200e-6;
    m_burstDelay = 0.0;
}

void SineGen_Block::SetParameters()
{
    if (!m_sineGen) {
        return;
    }

    m_sineGen->Amplitude = m_amplitude;
    m_sineGen->Offset = m_offset;
    m_sineGen->Frequency = m_frequency;
    m_sineGen->Phase = m_phase;
    m_sineGen->ShowAdvancedParams = m_showAdvancedParams;
    m_sineGen->SampleRateOption = m_sampleRateOption;
    m_sineGen->SampleRate = m_sampleRate;
    m_sineGen->InitialDelay = m_initialDelay;
    m_sineGen->BurstMode = m_burstMode;
    m_sineGen->BurstLength = m_burstLength;
    m_sineGen->BurstPeriod = m_burstPeriod;
    m_sineGen->BurstDelay = m_burstDelay;
}

bool SineGen_Block::Setup()
{
    Block::Setup();
    return true;
}

bool SineGen_Block::Run()
{
    const double sampleRate = (m_sampleRateOption == SineGen::TimedFromSampleRate)
        ? m_sampleRate
        : simulator_param.samplingRate;
    
    if (sampleRate <= 0.0) {
        return false;
    }

    if (m_frequency < 0.0 || m_frequency > sampleRate / 2.0) {
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
        + static_cast<double>(m_sineGen->GetCount()) / sampleRate;

    double y = 0.0;
    switch (m_burstMode) {
    case SineGen::OFF:
        if (t >= m_initialDelay) {
            y = m_amplitude * std::sin(2.0 * PI * m_frequency * (t - m_initialDelay) + m_phase) + m_offset;
        } else {
            y = 0.0;
        }
        break;
    case SineGen::Single:
        if (t >= m_initialDelay && t < m_initialDelay + m_burstDelay) {
            y = m_offset;
        } else if (t >= m_initialDelay + m_burstDelay && t < m_initialDelay + m_burstDelay + m_burstLength) {
            y = m_amplitude * std::sin(2.0 * PI * m_frequency * (t - m_initialDelay - m_burstDelay) + m_phase) + m_offset;
        } else {
            y = 0.0;
        }
        break;
    case SineGen::Multiple: {
        const double wt = std::fmod(t - m_initialDelay, m_burstPeriod);
        if (wt >= m_burstDelay && wt < m_burstDelay + m_burstLength) {
            y = m_amplitude * std::sin(2.0 * PI * m_frequency * (wt - m_burstDelay) + m_phase) + m_offset;
        } else {
            y = m_offset;
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

    m_sineGen->Advance();
    return true;
}

bool SineGen_Block::Initialize()
{
    SetBlockType(Block::BlockType::SOURCE);

    m_sineGen = std::make_unique<SineGen>();

    AddOutputPort("output", m_sineGen->output, 1, Block::DataType::TIMED_DOUBLE);

    SetDefaultParamters();
    simulator_param = getSimu();

    try { m_amplitude = std::stod(getParameter("Amplitude").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Amplitude', using default value."); }
    try { m_offset = std::stod(getParameter("Offset").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Offset', using default value."); }
    try { m_frequency = std::stod(getParameter("Frequency").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Frequency', using default value."); }
    try { m_phase = std::stod(getParameter("Phase").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Phase', using default value."); }
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

    const double outFs = (m_sampleRateOption == SineGen::TimedFromSampleRate) ? m_sampleRate : getSimu().samplingRate;
    m_sineGen->output.SetSampleRate(outFs);

    if (!m_sineGen->Setup()) {
        return false;
    }

    return true;
}

SineGen::SelectedShowAdvancedParams SineGen_Block::ConvertStringToShowAdvancedParams(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "no") {
        return SineGen::No;
    }
    if (lower == "yes" || lower == "1") {
        return SineGen::Yes;
    }
    return SineGen::No;
}

SineGen::SelectedSampleRateOption SineGen_Block::ConvertStringToSampleRateOption(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "untimed" || lower == "0") {
        return SineGen::UnTimed;
    }
    if (lower == "timed from samplerate" || lower == "timedfromsamplerate" || lower == "1") {
        return SineGen::TimedFromSampleRate;
    }
    if (lower == "timed from schematic" || lower == "timedfromschematic" || lower == "2") {
        return SineGen::TimedFromSchematic;
    }
    return SineGen::TimedFromSchematic;
}

SineGen::SelectedBurstMode SineGen_Block::ConvertStringToBurstMode(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "off") {
        return SineGen::OFF;
    }
    if (lower == "single" || lower == "1") {
        return SineGen::Single;
    }
    if (lower == "multiple" || lower == "2") {
        return SineGen::Multiple;
    }
    return SineGen::OFF;
}








