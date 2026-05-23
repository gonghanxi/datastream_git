#include "RADAR_FSK_Block.h"
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

RADAR_FSK_Block::RADAR_FSK_Block(const std::string& name)
    : Block(name)
{
}

void RADAR_FSK_Block::SetDefaultParamters()
{
    m_type = RADAR_FSK::FSK;
    m_pri = 1e-4;
    m_fhSequence.Resize(1, 3);
    m_fhSequence(0) = 1e6;
    m_fhSequence(1) = 2e6;
    m_fhSequence(2) = 3e6;
    m_fskpskSequence.Resize(1, 10);
    m_fskpskSequence(0) = 2e5;
    m_fskpskSequence(1) = 4e5;
    m_fskpskSequence(2) = 8e5;
    m_fskpskSequence(3) = 5e5;
    m_fskpskSequence(4) = 10e5;
    m_fskpskSequence(5) = 9e5;
    m_fskpskSequence(6) = 7e5;
    m_fskpskSequence(7) = 3e5;
    m_fskpskSequence(8) = 6e5;
    m_fskpskSequence(9) = 1e5;
    m_timeIntervals.Resize(1, 3);
    m_timeIntervals(0) = 1e-5;
    m_timeIntervals(1) = 1e-5;
    m_timeIntervals(2) = 1e-5;
    m_fskpskSubTimePeriod = 1e-5;
    m_codeLength = RADAR_FSK::Length_13;
    m_sampleRate = 10e6;
}

void RADAR_FSK_Block::SetParameters()
{
    if (!m_radarFsk) {
        return;
    }

    m_radarFsk->Type = m_type;
    m_radarFsk->PRI = m_pri;
    m_radarFsk->FHSequence = m_fhSequence;
    m_radarFsk->FSKPSKSequence = m_fskpskSequence;
    m_radarFsk->TimeIntervals = m_timeIntervals;
    m_radarFsk->FSKPSKSubTimePeriod = m_fskpskSubTimePeriod;
    m_radarFsk->CodeLength = m_codeLength;
    m_radarFsk->SampleRate = m_sampleRate;
}

bool RADAR_FSK_Block::Setup()
{
    Block::Setup();
    return true;
}

bool RADAR_FSK_Block::Run()
{

    if (!m_radarFsk) {
        return false;
    }

    if (!m_radarFsk->Run()) {
        return false;
    }

    std::vector<std::complex<double>> outputData;
    outputData.push_back(m_radarFsk->output[0U]);

    WriteOutputData(GetOutputPortName(0), outputData);

    return true;
}

bool RADAR_FSK_Block::Initialize()
{
    SetBlockType(Block::BlockType::SOURCE);

    m_radarFsk = std::make_unique<RADAR_FSK>();

    AddOutputPort("output", m_radarFsk->output, 1, Block::DataType::CIRCULAR_BUFFER_DCOMPLEX);

    SetDefaultParamters();

    try { m_type = ConvertStringToType(getParameter("Type").Value); } catch (...) { }
    try { m_pri = std::stod(getParameter("PRI").Value); } catch (...) { }
    try { m_fhSequence = DataTypesAndParsers::ParseStringToMatrixDouble(getParameter("FHSequence").Value); } catch (...) { }
    try { m_fskpskSequence = DataTypesAndParsers::ParseStringToMatrixDouble(getParameter("FSKPSKSequence").Value); } catch (...) { }
    try { m_timeIntervals = DataTypesAndParsers::ParseStringToMatrixDouble(getParameter("TimeIntervals").Value); } catch (...) { }
    try { m_fskpskSubTimePeriod = std::stod(getParameter("FSKPSKSubTimePeriod").Value); } catch (...) { }
    try { m_codeLength = ConvertStringToCodeLength(getParameter("CodeLength").Value); } catch (...) { }
    try { m_sampleRate = std::stod(getParameter("SampleRate").Value); } catch (...) { }

    SetParameters();

    if (!m_radarFsk->Initialize()) {
        return false;
    }

    if (!m_radarFsk->Setup()) {
        return false;
    }

    return true;
}

RADAR_FSK::Types RADAR_FSK_Block::ConvertStringToType(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "fsk") {
        return RADAR_FSK::FSK;
    }
    if (lower == "fsk_psk" || lower == "1") {
        return RADAR_FSK::FSK_PSK;
    }
    return RADAR_FSK::FSK;
}

RADAR_FSK::CodeLengthEnum RADAR_FSK_Block::ConvertStringToCodeLength(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "length_2_a") {
        return RADAR_FSK::Length_2_a;
    }
    if (lower == "length_2_b" || lower == "1") {
        return RADAR_FSK::Length_2_b;
    }
    if (lower == "length_3" || lower == "2") {
        return RADAR_FSK::Length_3;
    }
    if (lower == "length_4_a" || lower == "3") {
        return RADAR_FSK::Length_4_a;
    }
    if (lower == "length_4_b" || lower == "4") {
        return RADAR_FSK::Length_4_b;
    }
    if (lower == "length_5" || lower == "5") {
        return RADAR_FSK::Length_5;
    }
    if (lower == "length_7" || lower == "6") {
        return RADAR_FSK::Length_7;
    }
    if (lower == "length_11" || lower == "7") {
        return RADAR_FSK::Length_11;
    }
    if (lower == "length_13" || lower == "8") {
        return RADAR_FSK::Length_13;
    }
    return RADAR_FSK::Length_2_a;
}


