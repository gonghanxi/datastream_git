#include "IID_Uniform_Block.h"
#include <algorithm>
#include <cctype>
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

IID_Uniform_Block::IID_Uniform_Block(const std::string& name)
    : Block(name)
{
}

void IID_Uniform_Block::SetDefaultParamters()
{
    m_loLevel = 0.0;
    m_hiLevel = 1.0;
    m_showAdvancedParams = IID_Uniform::No;
    m_sampleRateOption = IID_Uniform::TimedFromSchematic;
    m_sampleRate = getSimu().samplingRate;
    m_initialDelay = 0;
    m_burstMode = IID_Uniform::OFF;
    m_burstLength = 100;
    m_burstPeriod = 200;
    m_burstDelay = 0;
}

void IID_Uniform_Block::SetParameters()
{
    if (!m_iidUniform) {
        return;
    }

    m_iidUniform->LoLevel = m_loLevel;
    m_iidUniform->HiLevel = m_hiLevel;
    m_iidUniform->ShowAdvancedParams = m_showAdvancedParams;
    m_iidUniform->SampleRateOption = m_sampleRateOption;
    m_iidUniform->SampleRate = m_sampleRate;
    m_iidUniform->InitialDelay = m_initialDelay;
    m_iidUniform->BurstMode = m_burstMode;
    m_iidUniform->BurstLength = m_burstLength;
    m_iidUniform->BurstPeriod = m_burstPeriod;
    m_iidUniform->BurstDelay = m_burstDelay;
}

bool IID_Uniform_Block::Setup()
{
    Block::Setup();
    return true;
}

bool IID_Uniform_Block::Run()
{
    if (!CanProcess()) {
        return false;
    }
if (!m_iidUniform) {
        return false;
    }

    if (!m_iidUniform->Run()) {
        return false;
    }

    std::vector<double> outputData;
    outputData.push_back(m_iidUniform->output[0U]);

    WriteOutputData(GetOutputPortName(0), outputData);

    if (m_iidUniform) {
        m_iidUniform->Advance();
    }

    return true;
}

bool IID_Uniform_Block::Initialize()
{
    SetBlockType(Block::BlockType::SOURCE);

    m_iidUniform = std::make_unique<IID_Uniform>();

    AddOutputPort("output", m_iidUniform->output, 1, Block::DataType::TIMED_DOUBLE);

    SetDefaultParamters();
    simulator_param = getSimu();

    try { m_loLevel = std::stod(getParameter("LoLevel").Value); } catch (...) { }
    try { m_hiLevel = std::stod(getParameter("HiLevel").Value); } catch (...) { }
    try { m_showAdvancedParams = ConvertStringToShowAdvancedParams(getParameter("ShowAdvancedParams").Value); } catch (...) { }
    try { m_sampleRateOption = ConvertStringToSampleRateOption(getParameter("SampleRateOption").Value); } catch (...) { }
    try { m_sampleRate = std::stod(getParameter("SampleRate").Value); } catch (...) { }
    
    if (m_sampleRate <= 0.0) {
        std::cout << "SampleRate must be greater than 0." << std::endl;
        return false;
    }
    try { m_initialDelay = std::stoi(getParameter("InitialDelay").Value); } catch (...) { }
    try { m_burstMode = ConvertStringToBurstMode(getParameter("BurstMode").Value); } catch (...) { }
    try { m_burstLength = std::stoi(getParameter("BurstLength").Value); } catch (...) { }
    try { m_burstPeriod = std::stoi(getParameter("BurstPeriod").Value); } catch (...) { }
    try { m_burstDelay = std::stoi(getParameter("BurstDelay").Value); } catch (...) { }

    SetParameters();

    if (!m_iidUniform->Setup()) {
        return false;
    }

    return true;
}

IID_Uniform::SelectedShowAdvancedParams IID_Uniform_Block::ConvertStringToShowAdvancedParams(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "no") {
        return IID_Uniform::No;
    }
    if (lower == "yes" || lower == "1") {
        return IID_Uniform::Yes;
    }
    return IID_Uniform::No;
}

IID_Uniform::SelectedSampleRateOption IID_Uniform_Block::ConvertStringToSampleRateOption(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "untimed" || lower == "0") {
        return IID_Uniform::UnTimed;
    }
    if (lower == "timed from samplerate" || lower == "timedfromsamplerate" || lower == "1") {
        return IID_Uniform::TimedFromSampleRate;
    }
    if (lower == "timed from schematic" || lower == "timedfromschematic" || lower == "2") {
        return IID_Uniform::TimedFromSchematic;
    }
    return IID_Uniform::TimedFromSchematic;
}

IID_Uniform::SelectedBurstMode IID_Uniform_Block::ConvertStringToBurstMode(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "off") {
        return IID_Uniform::OFF;
    }
    if (lower == "single" || lower == "1") {
        return IID_Uniform::Single;
    }
    if (lower == "multiple" || lower == "2") {
        return IID_Uniform::Multiple;
    }
    return IID_Uniform::OFF;
}


















