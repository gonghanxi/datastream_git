#include "Ramp_Block.h"
#include <algorithm>
#include <cctype>
#include <vector>
#include <iostream>

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

Ramp_Block::Ramp_Block(const std::string& name)
    : Block(name)
{
}

void Ramp_Block::SetDefaultParamters()
{
    m_stepPerSample = 1.0;
    m_initialValue = 0.0;
    m_showAdvancedParams = Ramp::No;
    m_sampleRateOption = Ramp::TimedFromSchematic;
    m_sampleRate = getSimu().samplingRate;
    m_initialDelay = 0;
    m_currentValue = 0.0;
    m_rampCount = 0;
}

void Ramp_Block::SetParameters()
{
    if (!m_ramp) {
        return;
    }

    m_ramp->StepPerSample = m_stepPerSample;
    m_ramp->InitialValue = m_initialValue;
    m_ramp->ShowAdvancedParams = m_showAdvancedParams;
    m_ramp->SampleRateOption = m_sampleRateOption;
    m_ramp->SampleRate = m_sampleRate;
    m_ramp->InitialDelay = m_initialDelay;
}

bool Ramp_Block::Setup()
{
    Block::Setup();
    return true;
}

bool Ramp_Block::Run()
{
    double y;
    if (m_rampCount < m_initialDelay)
    {
        y = 0.0;
    }
    else if (m_rampCount == m_initialDelay)
    {
        y = m_initialValue;
        m_currentValue = m_initialValue;
    }
    else
    {
        m_currentValue += m_stepPerSample;
        y = m_currentValue;
    }

    std::vector<double> outputData;
    outputData.push_back(y);

    WriteOutputData(GetOutputPortName(0), outputData);

    m_rampCount++;

    return true;
}

bool Ramp_Block::Initialize()
{
    SetBlockType(Block::BlockType::SOURCE);

    m_ramp = std::make_unique<Ramp>();

    AddOutputPort("output", m_ramp->output, 1, Block::DataType::TIMED_DOUBLE);

    SetDefaultParamters();
    simulator_param = getSimu();

    try { m_stepPerSample = std::stod(getParameter("StepPerSample").Value); } catch (...) { }
    try { m_initialValue = std::stod(getParameter("InitialValue").Value); } catch (...) { }
    try { m_showAdvancedParams = ConvertStringToShowAdvancedParams(getParameter("ShowAdvancedParams").Value); } catch (...) { }
    try { m_sampleRateOption = ConvertStringToSampleRateOption(getParameter("SampleRateOption").Value); } catch (...) { }
    try { m_sampleRate = std::stod(getParameter("SampleRate").Value); } catch (...) { }

    if (m_sampleRate <= 0.0) {
        std::cout << "SampleRate must be greater than 0." << std::endl;
        return false;
    }
    try { m_initialDelay = std::stoi(getParameter("InitialDelay").Value); } catch (...) { }

    SetParameters();

    if (!m_ramp->Setup()) {
        return false;
    }

    return true;
}

Ramp::SelectedShowAdvancedParams Ramp_Block::ConvertStringToShowAdvancedParams(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "no") {
        return Ramp::No;
    }
    if (lower == "yes" || lower == "1") {
        return Ramp::Yes;
    }
    return Ramp::No;
}

Ramp::SelectedSampleRateOption Ramp_Block::ConvertStringToSampleRateOption(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "untimed" || lower == "0") {
        return Ramp::UnTimed;
    }
    if (lower == "timed from samplerate" || lower == "timedfromsamplerate" || lower == "1") {
        return Ramp::TimedFromSampleRate;
    }
    if (lower == "timed from schematic" || lower == "timedfromschematic" || lower == "2") {
        return Ramp::TimedFromSchematic;
    }
    return Ramp::TimedFromSchematic;
}
















