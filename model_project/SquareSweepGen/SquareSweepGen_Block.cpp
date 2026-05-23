#include "SquareSweepGen_Block.h"

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

SquareSweepGen_Block::SquareSweepGen_Block(const std::string& name)
    : Block(name)
    , m_loLevel(0.0)
    , m_hiLevel(1.0)
    , m_fSweepType(SquareSweepGen::linear)
    , m_startFreq(1e3)
    , m_stopFreq(10e3)
    , m_phase(0.0)
    , m_sweepPeriod(0.0)
    , m_dutyCycle(50.0)
    , m_polarity(SquareSweepGen::normal)
    , m_showAdvancedParams(SquareSweepGen::No)
    , m_sampleRateOption(SquareSweepGen::TimedFromSchematic)
    , m_sampleRate(0.0)
    , m_initialDelay(0.0)
{
}

void SquareSweepGen_Block::SetDefaultParamters()
{
    m_loLevel = 0.0;
    m_hiLevel = 1.0;
    m_fSweepType = SquareSweepGen::linear;
    m_startFreq = 1e3;
    m_stopFreq = 10e3;
    m_phase = 0.0;
    m_sweepPeriod = getSimu().stopTime + getSimu().time_Interval;
    m_dutyCycle = 50.0;
    m_polarity = SquareSweepGen::normal;
    m_showAdvancedParams = SquareSweepGen::No;
    m_sampleRateOption = SquareSweepGen::TimedFromSchematic;
    m_sampleRate = getSimu().samplingRate;
    m_initialDelay = 0.0;
}

void SquareSweepGen_Block::SetParameters()
{
    if (!m_squareSweepGen) {
        return;
    }

    m_squareSweepGen->LoLevel = m_loLevel;
    m_squareSweepGen->HiLevel = m_hiLevel;
    m_squareSweepGen->FSweepType = m_fSweepType;
    m_squareSweepGen->StartFreq = m_startFreq;
    m_squareSweepGen->StopFreq = m_stopFreq;
    m_squareSweepGen->Phase = m_phase;
    m_squareSweepGen->SweepPeriod = m_sweepPeriod;
    m_squareSweepGen->DutyCycle = m_dutyCycle;
    m_squareSweepGen->Polarity = m_polarity;
    m_squareSweepGen->ShowAdvancedParams = m_showAdvancedParams;
    m_squareSweepGen->SampleRateOption = m_sampleRateOption;
    m_squareSweepGen->SampleRate = m_sampleRate;
    m_squareSweepGen->InitialDelay = m_initialDelay;
}

bool SquareSweepGen_Block::Setup()
{
    Block::Setup();
    return true;
}

bool SquareSweepGen_Block::Run()
{
    if (!CanProcess()) {
        return false;
    }

    const double sampleRate = (m_sampleRateOption == SquareSweepGen::TimedFromSampleRate)
        ? m_sampleRate
        : simulator_param.samplingRate;
    
    if (sampleRate <= 0.0) {
        return false;
    }

    if (m_startFreq <= 0.0 || m_startFreq > sampleRate / 4.0) {
        return false;
    }
    if (m_stopFreq <= 0.0 || m_stopFreq > sampleRate / 4.0) {
        return false;
    }
    if (m_sweepPeriod < 1.0 / sampleRate) {
        return false;
    }
    const double maxFreq = std::max(m_startFreq, m_stopFreq);
    if (m_dutyCycle < 100.0 * maxFreq / sampleRate || m_dutyCycle > 100.0 * (1.0 - maxFreq / sampleRate)) {
        return false;
    }

    const double PI = std::acos(-1.0);
    const double t = simulator_param.startTime
        + static_cast<double>(m_squareSweepGen->GetCount()) / sampleRate
        + 1e-16;

    double outVal = 0.0;
    double freqVal = 0.0;

    if (t < m_initialDelay) {
        outVal = 0.0;
        freqVal = 0.0;
    } else {
        const double tsweep = t - m_initialDelay;
        double Fsweep = 0.0;
        switch (m_fSweepType) {
        case SquareSweepGen::linear:
            freqVal = m_startFreq + (m_stopFreq - m_startFreq) * (tsweep / m_sweepPeriod);
            Fsweep = m_startFreq + (m_stopFreq - m_startFreq) * (tsweep / m_sweepPeriod) / 2.0;
            break;
        case SquareSweepGen::log:
            freqVal = m_startFreq * std::pow(m_stopFreq / m_startFreq, tsweep / m_sweepPeriod);
            Fsweep = m_startFreq * m_sweepPeriod / (tsweep * std::log(m_stopFreq / m_startFreq))
                * (std::pow(m_stopFreq / m_startFreq, tsweep / m_sweepPeriod) - 1.0);
            break;
        default:
            break;
        }

        const double tEdge = m_dutyCycle / 100.0 / Fsweep;
        const double phaseD = std::fmod((std::fmod(m_phase, 2.0 * PI) + 2.0 * PI), 2.0 * PI);
        const double pt = std::fmod((t - m_initialDelay + phaseD / (2.0 * PI) / Fsweep), 1.0 / Fsweep);

        if (pt < tEdge) {
            outVal = (m_polarity == SquareSweepGen::inverted) ? m_loLevel : m_hiLevel;
        } else {
            outVal = (m_polarity == SquareSweepGen::inverted) ? m_hiLevel : m_loLevel;
        }
    }

    std::vector<double> outData;
    outData.push_back(outVal);
    WriteOutputData(GetOutputPortName(0), outData);

    std::vector<double> freqData;
    freqData.push_back(freqVal);
    WriteOutputData(GetOutputPortName(1), freqData);

    m_squareSweepGen->Advance();
    return true;
}

bool SquareSweepGen_Block::Initialize()
{
    SetBlockType(Block::BlockType::SOURCE);

    m_squareSweepGen = std::make_unique<SquareSweepGen>();

    AddOutputPort("output", m_squareSweepGen->output, 1, Block::DataType::TIMED_DOUBLE);
    AddOutputPort("frequency", m_squareSweepGen->frequency, 1, Block::DataType::TIMED_DOUBLE);

    SetDefaultParamters();

    simulator_param = getSimu();

    try { m_loLevel = std::stod(getParameter("LoLevel").Value); } catch (...) { }
    try { m_hiLevel = std::stod(getParameter("HiLevel").Value); } catch (...) { }
    try { m_fSweepType = ConvertStringToFSweepType(getParameter("FSweepType").Value); } catch (...) { }
    try { m_startFreq = std::stod(getParameter("StartFreq").Value); } catch (...) { }
    try { m_stopFreq = std::stod(getParameter("StopFreq").Value); } catch (...) { }
    try { m_phase = std::stod(getParameter("Phase").Value); } catch (...) { }
    try { m_sweepPeriod = std::stod(getParameter("SweepPeriod").Value); } catch (...) { }
    try { m_dutyCycle = std::stod(getParameter("DutyCycle").Value); } catch (...) { }
    try { m_polarity = ConvertStringToPolarity(getParameter("Polarity").Value); } catch (...) { }
    try { m_showAdvancedParams = ConvertStringToShowAdvanced(getParameter("ShowAdvancedParams").Value); } catch (...) { }
    try { m_sampleRateOption = ConvertStringToSampleRateOption(getParameter("SampleRateOption").Value); } catch (...) { }
    try { m_sampleRate = std::stod(getParameter("SampleRate").Value); } catch (...) { }
    
    if (m_sampleRate <= 0.0) {
        std::cout << "SampleRate must be greater than 0." << std::endl;
        return false;
    }
    try { m_initialDelay = std::stod(getParameter("InitialDelay").Value); } catch (...) { }

    SetParameters();

    const double outFs = (m_sampleRateOption == SquareSweepGen::TimedFromSampleRate) ? m_sampleRate : getSimu().samplingRate;
    m_squareSweepGen->output.SetSampleRate(outFs);

    return m_squareSweepGen->Setup();
}

SquareSweepGen::SelectedFSweepType SquareSweepGen_Block::ConvertStringToFSweepType(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "linear") {
        return SquareSweepGen::linear;
    }
    if (lower == "log" || lower == "1") return SquareSweepGen::log;
    return SquareSweepGen::linear;
}

SquareSweepGen::SelectedPolarity SquareSweepGen_Block::ConvertStringToPolarity(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "normal") {
        return SquareSweepGen::normal;
    }
    if (lower == "inverted" || lower == "1") return SquareSweepGen::inverted;
    return SquareSweepGen::normal;
}

SquareSweepGen::SelectedShowAdvancedParams SquareSweepGen_Block::ConvertStringToShowAdvanced(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "no") {
        return SquareSweepGen::No;
    }
    if (lower == "yes" || lower == "1") return SquareSweepGen::Yes;
    return SquareSweepGen::No;
}

SquareSweepGen::SelectedSampleRateOption SquareSweepGen_Block::ConvertStringToSampleRateOption(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "untimed" || lower == "0") return SquareSweepGen::UnTimed;
    if (lower == "timedfromsamplerate" || lower == "1") return SquareSweepGen::TimedFromSampleRate;
    if (lower == "timed from samplerate") return SquareSweepGen::TimedFromSampleRate;
    if (lower == "timedfromschematic" || lower == "2") return SquareSweepGen::TimedFromSchematic;
    if (lower == "timed from schematic") return SquareSweepGen::TimedFromSchematic;
    return SquareSweepGen::TimedFromSchematic;
}









