#include "GaussianNoiseGen_Block.h"

#include <algorithm>
#include <cctype>
#include <cmath>
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

GaussianNoiseGen_Block::GaussianNoiseGen_Block(const std::string& name)
    : Block(name)
    , m_nDensity(4.00388587e-21)
    , m_refR(50.0)
    , m_showAdvancedParams(GaussianNoiseGen::No)
    , m_sampleRateOption(GaussianNoiseGen::TimedFromSchematic)
    , m_sampleRate(0.0)
    , m_initialDelay(0)
    , m_burstMode(GaussianNoiseGen::OFF)
    , m_burstLength(100)
    , m_burstPeriod(200)
    , m_burstDelay(0)
{
}

void GaussianNoiseGen_Block::SetDefaultParamters()
{
    m_nDensity = 4.00388587e-21;
    m_refR = 50.0;
    m_showAdvancedParams = GaussianNoiseGen::No;
    m_sampleRateOption = GaussianNoiseGen::TimedFromSchematic;
    m_sampleRate = getSimu().samplingRate;
    m_initialDelay = 0;
    m_burstMode = GaussianNoiseGen::OFF;
    m_burstLength = 100;
    m_burstPeriod = 200;
    m_burstDelay = 0;
}

void GaussianNoiseGen_Block::SetParameters()
{
    if (!m_gaussian) {
        return;
    }

    m_gaussian->NDensity = m_nDensity;
    m_gaussian->RefR = m_refR;
    m_gaussian->ShowAdvancedParams = m_showAdvancedParams;
    m_gaussian->SampleRateOption = m_sampleRateOption;
    m_gaussian->SampleRate = m_sampleRate;
    m_gaussian->InitialDelay = m_initialDelay;
    m_gaussian->BurstMode = m_burstMode;
    m_gaussian->BurstLength = m_burstLength;
    m_gaussian->BurstPeriod = m_burstPeriod;
    m_gaussian->BurstDelay = m_burstDelay;
}

bool GaussianNoiseGen_Block::Setup()
{
    Block::Setup();
    return true;
}

bool GaussianNoiseGen_Block::Run()
{
    if (!m_gaussian) {
        return false;
    }

    if (!m_gaussian->Run()) {
        return false;
    }

    std::vector<double> outputData;
    outputData.push_back(m_gaussian->output[0U]);
    WriteOutputData(GetOutputPortName(0), outputData);

    m_gaussian->Advance();
    return true;
}

bool GaussianNoiseGen_Block::Initialize()
{
    SetBlockType(Block::BlockType::SOURCE);

    m_gaussian = std::make_unique<GaussianNoiseGen>();

    AddOutputPort("output", m_gaussian->output, 1, Block::DataType::TIMED_DOUBLE);

    SetDefaultParamters();

    simulator_param = getSimu();

    try { m_nDensity = std::stod(getParameter("NDensity").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'NDensity', using default value."); }
    try { m_refR = std::stod(getParameter("RefR").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'RefR', using default value."); }
    try { m_showAdvancedParams = ConvertStringToShowAdvanced(getParameter("ShowAdvancedParams").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'ShowAdvancedParams', using default value."); }
    try { m_sampleRateOption = ConvertStringToSampleRateOption(getParameter("SampleRateOption").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'SampleRateOption', using default value."); }
    if(m_sampleRateOption == GaussianNoiseGen::TimedFromSampleRate) {
        try { m_sampleRate = std::stod(getParameter("SampleRate").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'SampleRate', using default value."); }
    }
    try { m_initialDelay = std::stoi(getParameter("InitialDelay").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'InitialDelay', using default value."); }
    try { m_burstMode = ConvertStringToBurstMode(getParameter("BurstMode").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'BurstMode', using default value."); }
    try { m_burstLength = std::stoi(getParameter("BurstLength").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'BurstLength', using default value."); }
    try { m_burstPeriod = std::stoi(getParameter("BurstPeriod").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'BurstPeriod', using default value."); }
    try { m_burstDelay = std::stoi(getParameter("BurstDelay").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'BurstDelay', using default value."); }

    SetParameters();
    //m_gaussian->output.SetSampleRate(getSimu().samplingRate);
    m_gaussian->output.SetSampleRate(m_sampleRate);
    return m_gaussian->Setup();
}

GaussianNoiseGen::SelectedShowAdvancedParams GaussianNoiseGen_Block::ConvertStringToShowAdvanced(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "no") {
        return GaussianNoiseGen::No;
    }
    if (lower == "yes" || lower == "1") return GaussianNoiseGen::Yes;
    return GaussianNoiseGen::No;
}

GaussianNoiseGen::SelectedSampleRateOption GaussianNoiseGen_Block::ConvertStringToSampleRateOption(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "untimed" || lower == "0") return GaussianNoiseGen::UnTimed;
    if (lower == "timedfromsamplerate" || lower == "1") return GaussianNoiseGen::TimedFromSampleRate;
    if (lower == "timed from samplerate") return GaussianNoiseGen::TimedFromSampleRate;
    if (lower == "timedfromschematic" || lower == "2") return GaussianNoiseGen::TimedFromSchematic;
    if (lower == "timed from schematic") return GaussianNoiseGen::TimedFromSchematic;
    return GaussianNoiseGen::TimedFromSchematic;
}

GaussianNoiseGen::SelectedBurstMode GaussianNoiseGen_Block::ConvertStringToBurstMode(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "off") {
        return GaussianNoiseGen::OFF;
    }
    if (lower == "single" || lower == "1") return GaussianNoiseGen::Single;
    if (lower == "multiple" || lower == "2") return GaussianNoiseGen::Multiple;
    return GaussianNoiseGen::OFF;
}














