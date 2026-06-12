#include "ChirpGen_Block.h"

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

ChirpGen_Block::ChirpGen_Block(const std::string& name)
    : Block(name)
    , m_amplitude(1.0)
    , m_offset(0.0)
    , m_startFreq(1e3)
    , m_stopFreq(10e3)
    , m_phase(0.0)
    , m_sweepPeriod(0.0)
    , m_showAdvancedParams(ChirpGen::NO)
    , m_sampleRateOption(ChirpGen::TimedfromSchematic)
    , m_sampleRate(0.0)
    , m_initialDelay(0.0)
    , m_counter(0)
{
}

void ChirpGen_Block::SetDefaultParamters()
{
    m_amplitude = 1.0;
    m_offset = 0.0;
    m_startFreq = 1e3;
    m_stopFreq = 10e3;
    m_phase = 0.0;
    m_sweepPeriod = getSimu().stopTime + getSimu().time_Interval;
    m_showAdvancedParams = ChirpGen::NO;
    m_sampleRateOption = ChirpGen::TimedfromSchematic;
    m_sampleRate = getSimu().samplingRate;
    m_initialDelay = 0.0;
    m_counter = 0;
}

void ChirpGen_Block::SetParameters()
{
    if (!m_chirpGen) {
        return;
    }

    m_chirpGen->Amplitude = m_amplitude;
    m_chirpGen->Offset = m_offset;
    m_chirpGen->StartFreq = m_startFreq;
    m_chirpGen->StopFreq = m_stopFreq;
    m_chirpGen->Phase = m_phase;
    m_chirpGen->SweepPeriod = m_sweepPeriod;
    m_chirpGen->ShowAdvancedParams = m_showAdvancedParams;
    m_chirpGen->SampleRateOption = m_sampleRateOption;
    m_chirpGen->SampleRate = m_sampleRate;
    m_chirpGen->InitialDelay = m_initialDelay;
    m_chirpGen->counter = m_counter;
}

bool ChirpGen_Block::Setup()
{
    Block::Setup();
    return true;
}

bool ChirpGen_Block::Run()
{
    if (!m_chirpGen) {
        return false;
    }

    if (!m_chirpGen->Run()) {
        return false;
    }

    std::vector<double> sigData;
    sigData.push_back(m_chirpGen->SigOutput[0U]);
    WriteOutputData(GetOutputPortName(0), sigData);

    std::vector<double> freqData;
    freqData.push_back(m_chirpGen->freqOutput[0U]);
    WriteOutputData(GetOutputPortName(1), freqData);

    m_chirpGen->Advance();
    return true;
}

bool ChirpGen_Block::Initialize()
{
    SetBlockType(Block::BlockType::SOURCE);

    m_chirpGen = std::make_unique<ChirpGen>();

    AddOutputPort("SigOutput", m_chirpGen->SigOutput, 1, Block::DataType::TIMED_DOUBLE);
    AddOutputPort("freqOutput", m_chirpGen->freqOutput, 1, Block::DataType::TIMED_DOUBLE);

    SetDefaultParamters();

    simulator_param = getSimu();

    try { m_amplitude = std::stod(getParameter("Amplitude").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Amplitude', using default value."); }
    try { m_offset = std::stod(getParameter("Offset").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Offset', using default value."); }
    try { m_startFreq = std::stod(getParameter("StartFreq").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'StartFreq', using default value."); }
    try { m_stopFreq = std::stod(getParameter("StopFreq").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'StopFreq', using default value."); }
    try { m_phase = std::stod(getParameter("Phase").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Phase', using default value."); }
    try { m_sweepPeriod = std::stod(getParameter("SweepPeriod").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'SweepPeriod', using default value."); }
    try { m_showAdvancedParams = ConvertStringToShowAdvanced(getParameter("ShowAdvancedParams").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'ShowAdvancedParams', using default value."); }
    try { m_sampleRateOption = ConvertStringToSampleRateOption(getParameter("SampleRateOption").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'SampleRateOption', using default value."); }
    try { m_sampleRate = std::stod(getParameter("SampleRate").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'SampleRate', using default value."); }
    
    if (m_sampleRate <= 0.0) {
        LOG_ERROR("SampleRate must be greater than 0.");
        return false;
    }
    try { m_initialDelay = std::stod(getParameter("InitialDelay").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'InitialDelay', using default value."); }

    SetParameters();

    return m_chirpGen->Setup();
}

ChirpGen::ShowAdvancedParamsEnum ChirpGen_Block::ConvertStringToShowAdvanced(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "no") {
        return ChirpGen::NO;
    }
    if (lower == "yes" || lower == "1") return ChirpGen::YES;
    return ChirpGen::NO;
}

ChirpGen::SampleRateOptionEnum ChirpGen_Block::ConvertStringToSampleRateOption(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "untimed" || lower == "0") return ChirpGen::UnTimed;
    if (lower == "timedfromsamplerate" || lower == "1") return ChirpGen::TimedfromSampleRate;
    if (lower == "timed from samplerate") return ChirpGen::TimedfromSampleRate;
    if (lower == "timedfromschematic" || lower == "2") return ChirpGen::TimedfromSchematic;
    if (lower == "timed from schematic") return ChirpGen::TimedfromSchematic;
    return ChirpGen::TimedfromSchematic;
}





