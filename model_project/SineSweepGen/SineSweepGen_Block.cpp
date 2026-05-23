#include "SineSweepGen_Block.h"

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

SineSweepGen_Block::SineSweepGen_Block(const std::string& name)
    : Block(name)
    , m_amplitude(1.0)
    , m_offset(0.0)
    , m_fSweepType(SineSweepGen::linear)
    , m_startFreq(1e3)
    , m_stopFreq(10e3)
    , m_phase(0.0)
    , m_sweepPeriod(0.0)
    , m_showAdvancedParams(SineSweepGen::No)
    , m_sampleRateOption(SineSweepGen::TimedFromSchematic)
    , m_sampleRate(0.0)
    , m_initialDelay(0.0)
{
}

void SineSweepGen_Block::SetDefaultParamters()
{
    m_amplitude = 1.0;
    m_offset = 0.0;
    m_fSweepType = SineSweepGen::linear;
    m_startFreq = 1e3;
    m_stopFreq = 10e3;
    m_phase = 0.0;
    m_sweepPeriod = getSimu().stopTime + getSimu().time_Interval;
    m_showAdvancedParams = SineSweepGen::No;
    m_sampleRateOption = SineSweepGen::TimedFromSchematic;
    m_sampleRate = getSimu().samplingRate;
    m_initialDelay = 0.0;
}

void SineSweepGen_Block::SetParameters()
{
    if (!m_sineSweepGen) {
        return;
    }

    m_sineSweepGen->Amplitude = m_amplitude;
    m_sineSweepGen->Offset = m_offset;
    m_sineSweepGen->FSweepType = m_fSweepType;
    m_sineSweepGen->StartFreq = m_startFreq;
    m_sineSweepGen->StopFreq = m_stopFreq;
    m_sineSweepGen->Phase = m_phase;
    m_sineSweepGen->SweepPeriod = m_sweepPeriod;
    m_sineSweepGen->ShowAdvancedParams = m_showAdvancedParams;
    m_sineSweepGen->SampleRateOption = m_sampleRateOption;
    m_sineSweepGen->SampleRate = m_sampleRate;
    m_sineSweepGen->InitialDelay = m_initialDelay;
}

bool SineSweepGen_Block::Setup()
{
    Block::Setup();
    return true;
}

bool SineSweepGen_Block::Run()
{
    if (!CanProcess()) {
        return false;
    }

    const double sampleRate = (m_sampleRateOption == SineSweepGen::TimedFromSampleRate)
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

    const double PI = std::acos(-1.0);
    const double t = simulator_param.startTime
        + static_cast<double>(m_sineSweepGen->GetCount()) / sampleRate
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
        case SineSweepGen::linear:
            freqVal = m_startFreq + (m_stopFreq - m_startFreq) * (tsweep / m_sweepPeriod);
            Fsweep = m_startFreq + (m_stopFreq - m_startFreq) * (tsweep / m_sweepPeriod) / 2.0;
            break;
        case SineSweepGen::log:
            freqVal = m_startFreq * std::pow(m_stopFreq / m_startFreq, tsweep / m_sweepPeriod);
            Fsweep = m_startFreq * m_sweepPeriod / (tsweep * std::log(m_stopFreq / m_startFreq))
                * (std::pow(m_stopFreq / m_startFreq, tsweep / m_sweepPeriod) - 1.0);
            break;
        default:
            break;
        }

        outVal = m_amplitude * std::sin(2.0 * PI * Fsweep * tsweep + m_phase) + m_offset;
    }

    std::vector<double> outData;
    outData.push_back(outVal);
    WriteOutputData(GetOutputPortName(0), outData);

    std::vector<double> freqData;
    freqData.push_back(freqVal);
    WriteOutputData(GetOutputPortName(1), freqData);

    m_sineSweepGen->Advance();
    return true;
}

bool SineSweepGen_Block::Initialize()
{
    SetBlockType(Block::BlockType::SOURCE);

    m_sineSweepGen = std::make_unique<SineSweepGen>();

    AddOutputPort("output", m_sineSweepGen->output, 1, Block::DataType::TIMED_DOUBLE);
    AddOutputPort("frequency", m_sineSweepGen->frequency, 1, Block::DataType::TIMED_DOUBLE);

    SetDefaultParamters();

    simulator_param = getSimu();

    try { m_amplitude = std::stod(getParameter("Amplitude").Value); } catch (...) { }
    try { m_offset = std::stod(getParameter("Offset").Value); } catch (...) { }
    try { m_fSweepType = ConvertStringToFSweepType(getParameter("FSweepType").Value); } catch (...) { }
    try { m_startFreq = std::stod(getParameter("StartFreq").Value); } catch (...) { }
    try { m_stopFreq = std::stod(getParameter("StopFreq").Value); } catch (...) { }
    try { m_phase = std::stod(getParameter("Phase").Value); } catch (...) { }
    try { m_sweepPeriod = std::stod(getParameter("SweepPeriod").Value); } catch (...) { }
    try { m_showAdvancedParams = ConvertStringToShowAdvanced(getParameter("ShowAdvancedParams").Value); } catch (...) { }
    try { m_sampleRateOption = ConvertStringToSampleRateOption(getParameter("SampleRateOption").Value); } catch (...) { }
    try { m_sampleRate = std::stod(getParameter("SampleRate").Value); } catch (...) { }
    
    if (m_sampleRate <= 0.0) {
        std::cout << "SampleRate must be greater than 0." << std::endl;
        return false;
    }
    try { m_initialDelay = std::stod(getParameter("InitialDelay").Value); } catch (...) { }

    SetParameters();

    const double outFs = (m_sampleRateOption == SineSweepGen::TimedFromSampleRate) ? m_sampleRate : getSimu().samplingRate;
    m_sineSweepGen->output.SetSampleRate(outFs);

    return m_sineSweepGen->Setup();
}

SineSweepGen::SelectedFSweepType SineSweepGen_Block::ConvertStringToFSweepType(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "linear") {
        return SineSweepGen::linear;
    }
    if (lower == "log" || lower == "1") return SineSweepGen::log;
    return SineSweepGen::linear;
}

SineSweepGen::SelectedShowAdvancedParams SineSweepGen_Block::ConvertStringToShowAdvanced(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "no") {
        return SineSweepGen::No;
    }
    if (lower == "yes" || lower == "1") return SineSweepGen::Yes;
    return SineSweepGen::No;
}

SineSweepGen::SelectedSampleRateOption SineSweepGen_Block::ConvertStringToSampleRateOption(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "untimed" || lower == "0") return SineSweepGen::UnTimed;
    if (lower == "timedfromsamplerate" || lower == "1") return SineSweepGen::TimedFromSampleRate;
    if (lower == "timed from samplerate") return SineSweepGen::TimedFromSampleRate;
    if (lower == "timedfromschematic" || lower == "2") return SineSweepGen::TimedFromSchematic;
    if (lower == "timed from schematic") return SineSweepGen::TimedFromSchematic;
    return SineSweepGen::TimedFromSchematic;
}









