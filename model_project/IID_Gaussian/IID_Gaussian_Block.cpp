#include "IID_Gaussian_Block.h"
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

IID_Gaussian_Block::IID_Gaussian_Block(const std::string& name)
    : Block(name)
{
}

void IID_Gaussian_Block::SetDefaultParamters()
{
    m_stdDev = 1.0;
    m_offset = 0.0;
    m_showAdvancedParams = IID_Gaussian::No;
    m_sampleRateOption = IID_Gaussian::TimedFromSchematic;
    m_sampleRate = getSimu().samplingRate;
    m_initialDelay = 0;
    m_burstMode = IID_Gaussian::OFF;
    m_burstLength = 100;
    m_burstPeriod = 200;
    m_burstDelay = 0;
}

void IID_Gaussian_Block::SetParameters()
{
    if (!m_iidGaussian) {
        return;
    }

    m_iidGaussian->StdDev = m_stdDev;
    m_iidGaussian->Offset = m_offset;
    m_iidGaussian->ShowAdvancedParams = m_showAdvancedParams;
    m_iidGaussian->SampleRateOption = m_sampleRateOption;
    m_iidGaussian->SampleRate = m_sampleRate;
    m_iidGaussian->InitialDelay = m_initialDelay;
    m_iidGaussian->BurstMode = m_burstMode;
    m_iidGaussian->BurstLength = m_burstLength;
    m_iidGaussian->BurstPeriod = m_burstPeriod;
    m_iidGaussian->BurstDelay = m_burstDelay;
}

bool IID_Gaussian_Block::Setup()
{
    Block::Setup();
    return true;
}

bool IID_Gaussian_Block::Run()
{
    if (!CanProcess()) {
        return false;
    }
    if (!m_iidGaussian) {
        return false;
    }

    if (!m_iidGaussian->Run()) {
        return false;
    }

    std::vector<double> outputData;
    outputData.push_back(m_iidGaussian->output[0U]);

    WriteOutputData(GetOutputPortName(0), outputData);

    if (m_iidGaussian) {
        m_iidGaussian->Advance();
    }

    return true;
}

bool IID_Gaussian_Block::Initialize()
{
    SetBlockType(Block::BlockType::SOURCE);

    m_iidGaussian = std::make_unique<IID_Gaussian>();

    AddOutputPort("output", m_iidGaussian->output, 1, Block::DataType::TIMED_DOUBLE);

    SetDefaultParamters();
    simulator_param = getSimu();

    try { m_stdDev = std::stod(getParameter("StdDev").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'StdDev', using default value."); }
    try { m_offset = std::stod(getParameter("Offset").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Offset', using default value."); }
    try { m_showAdvancedParams = ConvertStringToShowAdvancedParams(getParameter("ShowAdvancedParams").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'ShowAdvancedParams', using default value."); }
    try { m_sampleRateOption = ConvertStringToSampleRateOption(getParameter("SampleRateOption").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'SampleRateOption', using default value."); }
    try { m_sampleRate = std::stod(getParameter("SampleRate").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'SampleRate', using default value."); }
    
    if (m_sampleRate <= 0.0) {
        std::cout << "SampleRate must be greater than 0." << std::endl;
        return false;
    }
    try { m_initialDelay = std::stoi(getParameter("InitialDelay").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'InitialDelay', using default value."); }
    try { m_burstMode = ConvertStringToBurstMode(getParameter("BurstMode").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'BurstMode', using default value."); }
    try { m_burstLength = std::stoi(getParameter("BurstLength").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'BurstLength', using default value."); }
    try { m_burstPeriod = std::stoi(getParameter("BurstPeriod").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'BurstPeriod', using default value."); }
    try { m_burstDelay = std::stoi(getParameter("BurstDelay").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'BurstDelay', using default value."); }

    SetParameters();

    if (!m_iidGaussian->Setup()) {
        return false;
    }

    return true;
}

IID_Gaussian::SelectedShowAdvancedParams IID_Gaussian_Block::ConvertStringToShowAdvancedParams(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "no") {
        return IID_Gaussian::No;
    }
    if (lower == "yes" || lower == "1") {
        return IID_Gaussian::Yes;
    }
    return IID_Gaussian::No;
}

IID_Gaussian::SelectedSampleRateOption IID_Gaussian_Block::ConvertStringToSampleRateOption(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "untimed" || lower == "0") {
        return IID_Gaussian::UnTimed;
    }
    if (lower == "timed from samplerate" || lower == "timedfromsamplerate" || lower == "1") {
        return IID_Gaussian::TimedFromSampleRate;
    }
    if (lower == "timed from schematic" || lower == "timedfromschematic" || lower == "2") {
        return IID_Gaussian::TimedFromSchematic;
    }
    return IID_Gaussian::TimedFromSchematic;
}

IID_Gaussian::SelectedBurstMode IID_Gaussian_Block::ConvertStringToBurstMode(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "off") {
        return IID_Gaussian::OFF;
    }
    if (lower == "single" || lower == "1") {
        return IID_Gaussian::Single;
    }
    if (lower == "multiple" || lower == "2") {
        return IID_Gaussian::Multiple;
    }
    return IID_Gaussian::OFF;
}
















