#include "SquareGen_Block.h"
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

SquareGen_Block::SquareGen_Block(const std::string& name)
    : Block(name)
{
}

void SquareGen_Block::SetDefaultParamters()
{
    m_loLevel = 0.0;
    m_hiLevel = 1.0;
    m_frequency = 5e3;
    m_phase = 0.0;
    m_dutyCycle = 50.0;
    m_polarity = SquareGen::normal;
    m_showAdvancedParams = SquareGen::No;
    m_sampleRateOption = SquareGen::TimedFromSchematic;
    m_sampleRate = getSimu().samplingRate;
    m_initialDelay = 0.0;
    m_burstMode = SquareGen::OFF;
    m_burstLength = 100e-6;
    m_burstPeriod = 200e-6;
    m_burstDelay = 0.0;
}

void SquareGen_Block::SetParameters()
{
    if (!m_squareGen) {
        return;
    }

    m_squareGen->LoLevel = m_loLevel;
    m_squareGen->HiLevel = m_hiLevel;
    m_squareGen->Frequency = m_frequency;
    m_squareGen->Phase = m_phase;
    m_squareGen->DutyCycle = m_dutyCycle;
    m_squareGen->Polarity = m_polarity;
    m_squareGen->ShowAdvancedParams = m_showAdvancedParams;
    m_squareGen->SampleRateOption = m_sampleRateOption;
    m_squareGen->SampleRate = m_sampleRate;
    m_squareGen->InitialDelay = m_initialDelay;
    m_squareGen->BurstMode = m_burstMode;
    m_squareGen->BurstLength = m_burstLength;
    m_squareGen->BurstPeriod = m_burstPeriod;
    m_squareGen->BurstDelay = m_burstDelay;
}

bool SquareGen_Block::Setup()
{
    Block::Setup();
    return true;
}

bool SquareGen_Block::Run()
{
    if (!CanProcess()) {
        return false;
    }

    const double sampleRate = (m_sampleRateOption == SquareGen::TimedFromSampleRate)
        ? m_sampleRate
        : simulator_param.samplingRate;
    
    if (sampleRate <= 0.0) {
        return false;
    }

    if (m_frequency < 0.0 || m_frequency > sampleRate / 4.0) {
        return false;
    }

    if (m_dutyCycle < 100.0 * m_frequency / sampleRate || m_dutyCycle > 100.0 * (1.0 - m_frequency / sampleRate)) {
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
        + static_cast<double>(m_squareGen->GetCount()) / sampleRate
        + 1e-16;

    const double tEdge = m_dutyCycle / 100.0 / m_frequency;
    const double phaseD = std::fmod((std::fmod(m_phase, 2.0 * PI) + 2.0 * PI), 2.0 * PI);
    const double pt = std::fmod((t - m_initialDelay + phaseD / (2.0 * PI) / m_frequency), 1.0 / m_frequency);

    double y = 0.0;
    switch (m_burstMode) {
    case SquareGen::OFF:
        if (pt < tEdge) {
            y = (m_polarity == SquareGen::inverted) ? m_loLevel : m_hiLevel;
        } else {
            y = (m_polarity == SquareGen::inverted) ? m_hiLevel : m_loLevel;
        }
        break;
    case SquareGen::Single:
        if (t >= m_initialDelay && t < m_initialDelay + m_burstDelay) {
            y = (m_polarity == SquareGen::inverted) ? m_hiLevel : m_loLevel;
        } else if (t >= m_initialDelay + m_burstDelay && t < m_initialDelay + m_burstDelay + m_burstLength) {
            if (pt - m_burstDelay < tEdge) {
                y = (m_polarity == SquareGen::inverted) ? m_loLevel : m_hiLevel;
            } else {
                y = (m_polarity == SquareGen::inverted) ? m_hiLevel : m_loLevel;
            }
        } else {
            y = 0.0;
        }
        break;
    case SquareGen::Multiple: {
        const double wt = std::fmod(t - m_initialDelay - m_burstDelay, m_burstPeriod);
        if (t >= m_initialDelay && t < m_initialDelay + m_burstDelay) {
            y = (m_polarity == SquareGen::inverted) ? m_hiLevel : m_loLevel;
        } else if (wt <= m_burstLength) {
            if (pt - m_burstDelay < tEdge) {
                y = (m_polarity == SquareGen::inverted) ? m_loLevel : m_hiLevel;
            } else {
                y = (m_polarity == SquareGen::inverted) ? m_hiLevel : m_loLevel;
            }
        } else {
            y = (m_polarity == SquareGen::inverted) ? m_hiLevel : m_loLevel;
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

    m_squareGen->Advance();
    return true;
}

bool SquareGen_Block::Initialize()
{
    SetBlockType(Block::BlockType::SOURCE);

    m_squareGen = std::make_unique<SquareGen>();

    AddOutputPort("output", m_squareGen->output, 1, Block::DataType::TIMED_DOUBLE);

    SetDefaultParamters();
    simulator_param = getSimu();

    try { m_loLevel = std::stod(getParameter("LoLevel").Value); } catch (...) { }
    try { m_hiLevel = std::stod(getParameter("HiLevel").Value); } catch (...) { }
    try { m_frequency = std::stod(getParameter("Frequency").Value); } catch (...) { }
    try { m_phase = std::stod(getParameter("Phase").Value); } catch (...) { }
    try { m_dutyCycle = std::stod(getParameter("DutyCycle").Value); } catch (...) { }
    try { m_polarity = ConvertStringToPolarity(getParameter("Polarity").Value); } catch (...) { }
    try { m_showAdvancedParams = ConvertStringToShowAdvancedParams(getParameter("ShowAdvancedParams").Value); } catch (...) { }
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

    const double outFs = (m_sampleRateOption == SquareGen::TimedFromSampleRate) ? m_sampleRate : getSimu().samplingRate;
    m_squareGen->output.SetSampleRate(outFs);

    if (!m_squareGen->Setup()) {
        return false;
    }

    return true;
}

SquareGen::SelectedPolarity SquareGen_Block::ConvertStringToPolarity(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "normal") {
        return SquareGen::normal;
    }
    if (lower == "inverted" || lower == "1") {
        return SquareGen::inverted;
    }
    return SquareGen::normal;
}

SquareGen::SelectedShowAdvancedParams SquareGen_Block::ConvertStringToShowAdvancedParams(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "no") {
        return SquareGen::No;
    }
    if (lower == "yes" || lower == "1") {
        return SquareGen::Yes;
    }
    return SquareGen::No;
}

SquareGen::SelectedSampleRateOption SquareGen_Block::ConvertStringToSampleRateOption(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "untimed" || lower == "0") {
        return SquareGen::UnTimed;
    }
    if (lower == "timed from samplerate" || lower == "timedfromsamplerate" || lower == "1") {
        return SquareGen::TimedFromSampleRate;
    }
    if (lower == "timed from schematic" || lower == "timedfromschematic" || lower == "2") {
        return SquareGen::TimedFromSchematic;
    }
    return SquareGen::TimedFromSchematic;
}

SquareGen::SelectedBurstMode SquareGen_Block::ConvertStringToBurstMode(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "off") {
        return SquareGen::OFF;
    }
    if (lower == "single" || lower == "1") {
        return SquareGen::Single;
    }
    if (lower == "multiple" || lower == "2") {
        return SquareGen::Multiple;
    }
    return SquareGen::OFF;
}








