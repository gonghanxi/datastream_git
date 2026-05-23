#include "RADAR_BarkerCode_Block.h"
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

RADAR_BarkerCode_Block::RADAR_BarkerCode_Block(const std::string& name)
    : Block(name)
{
}

void RADAR_BarkerCode_Block::SetDefaultParamters()
{
    m_pri = 1e-4;
    m_subPulseWidth = 1e-6;
    m_codeLength = RADAR_BarkerCode::Length_13;
    m_sampleRate = 10e6;
}

void RADAR_BarkerCode_Block::SetParameters()
{
    if (!m_radarBarker) {
        return;
    }

    m_radarBarker->PRI = m_pri;
    m_radarBarker->SubPulseWidth = m_subPulseWidth;
    m_radarBarker->CodeLength = m_codeLength;
    m_radarBarker->SampleRate = m_sampleRate;
}

bool RADAR_BarkerCode_Block::Setup()
{
    Block::Setup();
    return true;
}

bool RADAR_BarkerCode_Block::Run()
{
    if (!CanProcess()) {
        return false;
    }

    if (!m_radarBarker) {
        return false;
    }

    if (!m_radarBarker->Run()) {
        return false;
    }

    std::vector<std::complex<double>> outputData;
    outputData.push_back(m_radarBarker->output[0U]);

    WriteOutputData(GetOutputPortName(0), outputData);

    return true;
}

bool RADAR_BarkerCode_Block::Initialize()
{
    SetBlockType(Block::BlockType::SOURCE);

    m_radarBarker = std::make_unique<RADAR_BarkerCode>();

    AddOutputPort("output", m_radarBarker->output, 1, Block::DataType::CIRCULAR_BUFFER_DCOMPLEX);

    SetDefaultParamters();

    try { m_pri = std::stod(getParameter("PRI").Value); } catch (...) { }
    try { m_subPulseWidth = std::stod(getParameter("SubPulseWidth").Value); } catch (...) { }
    try { m_codeLength = ConvertStringToCodeLength(getParameter("CodeLength").Value); } catch (...) { }
    try { m_sampleRate = std::stod(getParameter("SampleRate").Value); } catch (...) { }

    SetParameters();

    if (!m_radarBarker->Setup()) {
        return false;
    }

    return true;
}

RADAR_BarkerCode::CodeLengthEnum RADAR_BarkerCode_Block::ConvertStringToCodeLength(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "length_2_a") {
        return RADAR_BarkerCode::Length_2_a;
    }
    if (lower == "length_2_b" || lower == "1") {
        return RADAR_BarkerCode::Length_2_b;
    }
    if (lower == "length_3" || lower == "2") {
        return RADAR_BarkerCode::Length_3;
    }
    if (lower == "length_4_a" || lower == "3") {
        return RADAR_BarkerCode::Length_4_a;
    }
    if (lower == "length_4_b" || lower == "4") {
        return RADAR_BarkerCode::Length_4_b;
    }
    if (lower == "length_5" || lower == "5") {
        return RADAR_BarkerCode::Length_5;
    }
    if (lower == "length_7" || lower == "6") {
        return RADAR_BarkerCode::Length_7;
    }
    if (lower == "length_11" || lower == "7") {
        return RADAR_BarkerCode::Length_11;
    }
    if (lower == "length_13" || lower == "8") {
        return RADAR_BarkerCode::Length_13;
    }
    return RADAR_BarkerCode::Length_2_a;
}


