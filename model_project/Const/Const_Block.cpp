#include "Const_Block.h"
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

Const_Block::Const_Block(const std::string& name)
    : Block(name)
{
}

void Const_Block::SetDefaultParamters()
{
    m_value = 0.0;
    m_showAdvancedParams = Const::No;
    m_sampleRateOption = Const::TimedFromSchematic;
    m_sampleRate = getSimu().samplingRate;
    m_initialDelay = 0;
}

void Const_Block::SetParameters()
{
    if (!m_const) {
        return;
    }

    m_const->Value = m_value;
    m_const->ShowAdvancedParams = m_showAdvancedParams;
    m_const->SampleRateOption = m_sampleRateOption;
    m_const->SampleRate = m_sampleRate;
    m_const->InitialDelay = m_initialDelay;

}

bool Const_Block::Setup()
{
    Block::Setup();
    return true;
}

bool Const_Block::Run()
{
    if (!m_const->Run()) {
        return false;
    }

    std::vector<double> outputData;
    outputData.push_back(m_const->output[0U]);

    WriteOutputData(GetOutputPortName(0), outputData);
    if (m_const) {
        m_const->Advance();
    }

    qDebug() << "Const_Block::Run - outputData: " << outputData.size();

    return true;
}

bool Const_Block::Initialize()
{
    SetBlockType(Block::BlockType::SOURCE);

    m_const = std::make_unique<Const>();

    AddOutputPort("output", m_const->output, 1, Block::DataType::TIMED_DOUBLE);

    SetDefaultParamters();
    simulator_param = getSimu();

    try { m_value = std::stod(getParameter("Value").Value); } catch (...) { }
    try { m_showAdvancedParams = ConvertStringToShowAdvancedParams(getParameter("ShowAdvancedParams").Value); } catch (...) { }
    try { m_sampleRateOption = ConvertStringToSampleRateOption(getParameter("SampleRateOption").Value); } catch (...) { }
    try { m_sampleRate = std::stod(getParameter("SampleRate").Value); } catch (...) { }
    if (m_sampleRate <= 0.0) {
        std::cout << "SampleRate must be greater than 0." << std::endl;
        return false;
    }
    try { m_initialDelay = std::stoi(getParameter("InitialDelay").Value); } catch (...) { }

    SetParameters();

    m_const->output.SetRate(1U);
    m_const->output.SetStartTime(simulator_param.startTime);

    if (!m_const->Setup()) {
        return false;
    }

    return true;
}

Const::SelectedShowAdvancedParams Const_Block::ConvertStringToShowAdvancedParams(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "no") {
        return Const::No;
    }
    if (lower == "yes" || lower == "1") {
        return Const::Yes;
    }
    return Const::No;
}

Const::SelectedSampleRateOption Const_Block::ConvertStringToSampleRateOption(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "untimed" || lower == "0") {
        return Const::UnTimed;
    }
    if (lower == "timed from samplerate" || lower == "timedfromsamplerate" || lower == "1") {
        return Const::TimedFromSampleRate;
    }
    if (lower == "timed from schematic" || lower == "timedfromschematic" || lower == "2") {
        return Const::TimedFromSchematic;
    }
    return Const::TimedFromSchematic;
}
