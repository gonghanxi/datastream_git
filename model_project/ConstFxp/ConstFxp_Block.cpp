#include "ConstFxp_Block.h"
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

ConstFxp_Block::ConstFxp_Block(const std::string& name)
    : Block(name)
{
}

void ConstFxp_Block::SetDefaultParamters()
{
    m_value = 0.0;
    m_fxpPos = 4;
    m_showAdvancedParams = ConstFxp::No;
    m_sampleRateOption = ConstFxp::TimedFromSchematic;
    m_sampleRate = getSimu().samplingRate;
    m_initialDelay = 0;
}

void ConstFxp_Block::SetParameters()
{
    if (!m_constFxp) {
        return;
    }

    m_constFxp->Value = m_value;
    m_constFxp->FxpPos = m_fxpPos;
    m_constFxp->ShowAdvancedParams = m_showAdvancedParams;
    m_constFxp->SampleRateOption = m_sampleRateOption;
    m_constFxp->SampleRate = m_sampleRate;
    m_constFxp->InitialDelay = m_initialDelay;
}

bool ConstFxp_Block::Setup()
{
    Block::Setup();
    return true;
}

bool ConstFxp_Block::Run()
{
    if (!m_constFxp->Run()) {
        return false;
    }

    std::vector<double> outputData;
    outputData.push_back(m_constFxp->output[0U]);

    WriteOutputData(GetOutputPortName(0), outputData);
    if (m_constFxp) {
        m_constFxp->Advance();
    }

    qDebug() << "ConstFxp_Block::Run - outputData: " << outputData.size();

    return true;
}

bool ConstFxp_Block::Initialize()
{
    SetBlockType(Block::BlockType::SOURCE);

    m_constFxp = std::make_unique<ConstFxp>();

    AddOutputPort("output", m_constFxp->output, 1, Block::DataType::TIMED_DOUBLE);

    SetDefaultParamters();
    simulator_param = getSimu();

    try { m_value = std::stod(getParameter("Value").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Value', using default value."); }
    try { m_fxpPos = std::stoi(getParameter("FxpPos").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'FxpPos', using default value."); }
    try { m_showAdvancedParams = ConvertStringToShowAdvancedParams(getParameter("ShowAdvancedParams").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'ShowAdvancedParams', using default value."); }
    try { m_sampleRateOption = ConvertStringToSampleRateOption(getParameter("SampleRateOption").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'SampleRateOption', using default value."); }
    try { m_sampleRate = std::stod(getParameter("SampleRate").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'SampleRate', using default value."); }
    if (m_sampleRate <= 0.0) {
        std::cout << "SampleRate must be greater than 0." << std::endl;
        return false;
    }
    try { m_initialDelay = std::stoi(getParameter("InitialDelay").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'InitialDelay', using default value."); }

    SetParameters();

    m_constFxp->output.SetRate(1U);
    m_constFxp->output.SetStartTime(simulator_param.startTime);

    if (!m_constFxp->Setup()) {
        return false;
    }

    return true;
}

ConstFxp::SelectedShowAdvancedParams ConstFxp_Block::ConvertStringToShowAdvancedParams(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "no") {
        return ConstFxp::No;
    }
    if (lower == "yes" || lower == "1") {
        return ConstFxp::Yes;
    }
    return ConstFxp::No;
}

ConstFxp::SelectedSampleRateOption ConstFxp_Block::ConvertStringToSampleRateOption(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "untimed" || lower == "0") {
        return ConstFxp::UnTimed;
    }
    if (lower == "timed from samplerate" || lower == "timedfromsamplerate" || lower == "1") {
        return ConstFxp::TimedFromSampleRate;
    }
    if (lower == "timed from schematic" || lower == "timedfromschematic" || lower == "2") {
        return ConstFxp::TimedFromSchematic;
    }
    return ConstFxp::TimedFromSchematic;
}
