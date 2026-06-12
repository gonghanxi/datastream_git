#include "RADAR_NLFM_Block.h"
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

RADAR_NLFM_Block::RADAR_NLFM_Block(const std::string& name)
    : Block(name)
{
}

void RADAR_NLFM_Block::SetDefaultParamters()
{
    m_pulsewidth = 1e-5;
    m_pri = 1e-4;
    m_bandwidth = 5e6;
    m_sampleRate = 10e6;
    m_nlfType = RADAR_NLFM::Hamming;
    m_polyCoef.Resize(1, 1);
    m_polyCoef(0) = 0.426;
}

void RADAR_NLFM_Block::SetParameters()
{
    if (!m_radarNlfm) {
        return;
    }

    m_radarNlfm->Pulsewidth = m_pulsewidth;
    m_radarNlfm->PRI = m_pri;
    m_radarNlfm->Bandwidth = m_bandwidth;
    m_radarNlfm->SampleRate = m_sampleRate;
    m_radarNlfm->NLF_Type = m_nlfType;
    m_radarNlfm->Polynomial_Coef = m_polyCoef;
}

bool RADAR_NLFM_Block::Setup()
{
    Block::Setup();
    return true;
}

bool RADAR_NLFM_Block::Run()
{
    if (!CanProcess()) {
        return false;
    }

    if (!m_radarNlfm) {
        return false;
    }

    if (!m_radarNlfm->Run()) {
        return false;
    }

    std::vector<std::complex<double>> outputData;
    outputData.push_back(m_radarNlfm->output[0U]);

    WriteOutputData(GetOutputPortName(0), outputData);

    return true;
}

bool RADAR_NLFM_Block::Initialize()
{
    SetBlockType(Block::BlockType::SOURCE);

    m_radarNlfm = std::make_unique<RADAR_NLFM>();

    AddOutputPort("output", m_radarNlfm->output, 1, Block::DataType::CIRCULAR_BUFFER_DCOMPLEX);

    SetDefaultParamters();

    try { m_pulsewidth = std::stod(getParameter("Pulsewidth").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Pulsewidth', using default value."); }
    try { m_pri = std::stod(getParameter("PRI").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'PRI', using default value."); }
    try { m_bandwidth = std::stod(getParameter("Bandwidth").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Bandwidth', using default value."); }
    try { m_sampleRate = std::stod(getParameter("SampleRate").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'SampleRate', using default value."); }
    try { m_nlfType = ConvertStringToNLFType(getParameter("NLF_Type").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'NLF_Type', using default value."); }
    try { m_polyCoef = DataTypesAndParsers::ParseStringToMatrixDouble(getParameter("Polynomial_Coef").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Polynomial_Coef', using default value."); }

    SetParameters();

    if (!m_radarNlfm->Initialize()) {
        return false;
    }

    if (!m_radarNlfm->Setup()) {
        return false;
    }

    return true;
}

RADAR_NLFM::NLF_Types RADAR_NLFM_Block::ConvertStringToNLFType(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "hamming") {
        return RADAR_NLFM::Hamming;
    }
    if (lower == "cos4" || lower == "1") {
        return RADAR_NLFM::Cos4;
    }
    if (lower == "gauss" || lower == "2") {
        return RADAR_NLFM::Gauss;
    }
    if (lower == "polynomial" || lower == "3") {
        return RADAR_NLFM::Polynomial;
    }
    return RADAR_NLFM::Hamming;
}


