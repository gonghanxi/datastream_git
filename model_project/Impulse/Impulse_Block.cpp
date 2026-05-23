#include "Impulse_Block.h"

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

Impulse_Block::Impulse_Block(const std::string& name)
    : Block(name)
    , m_level(1.0)
    , m_scaleBySampleRate(Impulse::No)
    , m_showAdvancedParams(Impulse::No)
    , m_sampleRateOption(Impulse::TimedFromSchematic)
    , m_sampleRate(0.0)
    , m_initialDelay(0.0)
    , m_burstMode(Impulse::OFF)
    , m_burstLength(100e-6)
    , m_burstPeriod(200e-6)
    , m_burstDelay(0.0)
{
}

void Impulse_Block::SetDefaultParamters()
{
    m_level = 1.0;
    m_scaleBySampleRate = Impulse::No;
    m_showAdvancedParams = Impulse::No;
    m_sampleRateOption = Impulse::TimedFromSchematic;
    m_sampleRate = getSimu().samplingRate;
    m_initialDelay = 0.0;
    m_burstMode = Impulse::OFF;
    m_burstLength = 100e-6;
    m_burstPeriod = 200e-6;
    m_burstDelay = 0.0;
}

void Impulse_Block::SetParameters()
{
    if (!m_impulse) {
        return;
    }

    m_impulse->Level = m_level;
    m_impulse->ScaleBySampleRate = m_scaleBySampleRate;
    m_impulse->ShowAdvancedParams = m_showAdvancedParams;
    m_impulse->SampleRateOption = m_sampleRateOption;
    m_impulse->SampleRate = m_sampleRate;
    m_impulse->InitialDelay = m_initialDelay;
    m_impulse->BurstMode = m_burstMode;
    m_impulse->BurstLength = m_burstLength;
    m_impulse->BurstPeriod = m_burstPeriod;
    m_impulse->BurstDelay = m_burstDelay;
}

bool Impulse_Block::Setup()
{
    Block::Setup();
    return true;
}

bool Impulse_Block::Run()
{
    if (!CanProcess()) {
        return false;
    }

    const double sampleRate = (m_sampleRateOption == Impulse::TimedFromSampleRate)
        ? m_sampleRate
        : simulator_param.samplingRate;
    
    if (sampleRate <= 0.0) {
        return false;
    }

    if (m_burstLength < 1.0 / sampleRate) {
        return false;
    }

    if (m_burstPeriod < 1.0 / sampleRate) {
        return false;
    }

    const double t = simulator_param.startTime
        + static_cast<double>(m_impulse->GetCount()) / sampleRate;

    double y = 0.0;
    switch (m_burstMode) {
    case Impulse::OFF:
        if (std::abs(t - m_initialDelay) < 0.5 / sampleRate) {
            y = (m_scaleBySampleRate == Impulse::Yes) ? sampleRate : 1.0;
        } else {
            y = 0.0;
        }
        break;
    case Impulse::Single:
        if (std::abs(t - (m_initialDelay + m_burstDelay)) < 0.5 / sampleRate) {
            y = (m_scaleBySampleRate == Impulse::Yes) ? sampleRate : 1.0;
        } else {
            y = 0.0;
        }
        break;
    case Impulse::Multiple: {
        const double wt = std::fmod(t - m_initialDelay, m_burstPeriod);
        if (std::abs(wt - m_burstDelay) < 0.5 / sampleRate) {
            y = (m_scaleBySampleRate == Impulse::Yes) ? sampleRate : 1.0;
        } else {
            y = 0.0;
        }
        break;
    }
    default:
        break;
    }

    std::vector<double> outputData;
    outputData.push_back(y);
    WriteOutputData(GetOutputPortName(0), outputData);

    m_impulse->Advance();
    return true;
}

bool Impulse_Block::Initialize()
{
    SetBlockType(Block::BlockType::SOURCE);

    m_impulse = std::make_unique<Impulse>();

    AddOutputPort("output", m_impulse->output, 1, Block::DataType::TIMED_DOUBLE);

    SetDefaultParamters();

    simulator_param = getSimu();

    try { m_level = std::stod(getParameter("Level").Value); } catch (...) { }
    try { m_scaleBySampleRate = ConvertStringToNoOrYes(getParameter("ScaleBySampleRate").Value); } catch (...) { }
    try { m_showAdvancedParams = ConvertStringToNoOrYes(getParameter("ShowAdvancedParams").Value); } catch (...) { }
    try { m_sampleRateOption = ConvertStringToSampleRateOption(getParameter("SampleRateOption").Value); } catch (...) { }
    try { m_sampleRate = std::stod(getParameter("SampleRate").Value); } catch (...) { }
    
    if (m_sampleRate <= 0.0) {
        std::cout << "SampleRate must be greater than 0." << std::endl;
        return false;
    }
    try { m_initialDelay = std::stod(getParameter("InitialDelay").Value); } catch (...) { }
    try { m_burstMode = ConvertStringToBurstMode(getParameter("BurstMode").Value); } catch (...) { }
    try { m_burstLength = std::stod(getParameter("BurstLength").Value); } catch (...) { }
    try { m_burstPeriod = std::stod(getParameter("BurstPeriod").Value); } catch (...) { }
    try { m_burstDelay = std::stod(getParameter("BurstDelay").Value); } catch (...) { }

    SetParameters();

    const double outFs = (m_sampleRateOption == Impulse::TimedFromSampleRate) ? m_sampleRate : getSimu().samplingRate;
    m_impulse->output.SetSampleRate(outFs);

    return m_impulse->Setup();
}

Impulse::SelectedNoOrYes Impulse_Block::ConvertStringToNoOrYes(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "no") {
        return Impulse::No;
    }
    if (lower == "yes" || lower == "1") return Impulse::Yes;
    return Impulse::No;
}

Impulse::SelectedSampleRateOption Impulse_Block::ConvertStringToSampleRateOption(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "untimed" || lower == "0") return Impulse::UnTimed;
    if (lower == "timedfromsamplerate" || lower == "1") return Impulse::TimedFromSampleRate;
    if (lower == "timed from samplerate") return Impulse::TimedFromSampleRate;
    if (lower == "timedfromschematic" || lower == "2") return Impulse::TimedFromSchematic;
    if (lower == "timed from schematic") return Impulse::TimedFromSchematic;
    return Impulse::TimedFromSchematic;
}

Impulse::SelectedBurstMode Impulse_Block::ConvertStringToBurstMode(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "off") {
        return Impulse::OFF;
    }
    if (lower == "single" || lower == "1") return Impulse::Single;
    if (lower == "multiple" || lower == "2") return Impulse::Multiple;
    return Impulse::OFF;
}









