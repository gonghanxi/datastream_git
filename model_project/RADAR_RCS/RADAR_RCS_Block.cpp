#include "RADAR_RCS_Block.h"

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

RADAR_RCS_Block::RADAR_RCS_Block(const std::string& name)
    : Block(name)
    , m_type(RADAR_RCS::GaussianPDF)
    , m_va(1.0)
    , m_vb(1.0)
    , m_tStep(0.0001)
    , m_durationTime(1.0)
{
}

void RADAR_RCS_Block::SetDefaultParamters()
{
    m_type = RADAR_RCS::GaussianPDF;
    m_va = 1.0;
    m_vb = 1.0;
    m_tStep = 0.0001;
    m_durationTime = 1.0;
}

void RADAR_RCS_Block::SetParameters()
{
    if (!m_radarRcs) {
        return;
    }

    m_radarRcs->Type = m_type;
    m_radarRcs->VA = m_va;
    m_radarRcs->VB = m_vb;
    m_radarRcs->TStep = m_tStep;
    m_radarRcs->DurationTime = m_durationTime;
}

bool RADAR_RCS_Block::Setup()
{
    Block::Setup();
    return true;
}

bool RADAR_RCS_Block::Run()
{
    if (!CanProcess()) {
        return false;
    }

    if (!m_radarRcs) {
        return false;
    }

    if (!m_radarRcs->Run()) {
        return false;
    }

    std::vector<SystemVueModelBuilder::EnvelopeSignal> esData;
    esData.push_back(m_radarRcs->Es[0U]);
    WriteOutputData(GetOutputPortName(0), esData);

    std::vector<double> rcsData;
    rcsData.push_back(m_radarRcs->RCS[0U]);
    WriteOutputData(GetOutputPortName(1), rcsData);

    m_radarRcs->Advance();
    return true;
}

bool RADAR_RCS_Block::Initialize()
{
    SetBlockType(Block::BlockType::SOURCE);

    m_radarRcs = std::make_unique<RADAR_RCS>();

    AddOutputPort("Es", m_radarRcs->Es, 1, Block::DataType::ENVELOPE_SIGNAL);
    AddOutputPort("RCS", m_radarRcs->RCS, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);

    SetDefaultParamters();

    try { m_type = ConvertStringToType(getParameter("Type").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Type', using default value."); }
    try { m_va = std::stod(getParameter("VA").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'VA', using default value."); }
    try { m_vb = std::stod(getParameter("VB").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'VB', using default value."); }
    try { m_tStep = std::stod(getParameter("TStep").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'TStep', using default value."); }
    try { m_durationTime = std::stod(getParameter("DurationTime").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'DurationTime', using default value."); }

    SetParameters();

    return true;
}

RADAR_RCS::SelectedType RADAR_RCS_Block::ConvertStringToType(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "constvalue") {
        return RADAR_RCS::ConstValue;
    }
    if (lower == "uniformpdf") {
        return RADAR_RCS::UniformPDF;
    }
    if (lower == "gaussianpdf") {
        return RADAR_RCS::GaussianPDF;
    }
    if (lower == "rayleighpdf") {
        return RADAR_RCS::RayleighPDF;
    }
    if (lower == "lognormalpdf") {
        return RADAR_RCS::LogNormalPDF;
    }
    if (lower == "exponentialpdf") {
        return RADAR_RCS::ExponentialPDF;
    }
    if (lower == "weibullpdf") {
        return RADAR_RCS::WeibullPDF;
    }
    if (lower == "chisquaredpdf") {
        return RADAR_RCS::ChiSquaredPDF;
    }
    if (lower == "gammapdf") {
        return RADAR_RCS::GammaPDF;
    }
    if (lower == "betapdf") {
        return RADAR_RCS::BetaPDF;
    }
    if (lower == "fpdf") {
        return RADAR_RCS::FPDF;
    }
    if (lower == "binomialcdf") {
        return RADAR_RCS::BinomialCDF;
    }
    if (lower == "poissoncdf") {
        return RADAR_RCS::PoissonCDF;
    }
    if (lower == "gaussianpdf") {
        return RADAR_RCS::GaussianPDF;
    }
    if (lower == "const value" || lower == "0") return RADAR_RCS::ConstValue;
    if (lower == "uniform pdf" || lower == "1") return RADAR_RCS::UniformPDF;
    if (lower == "gaussian pdf" || lower == "2") return RADAR_RCS::GaussianPDF;
    if (lower == "rayleigh pdf" || lower == "3") return RADAR_RCS::RayleighPDF;
    if (lower == "lognormal pdf" || lower == "4") return RADAR_RCS::LogNormalPDF;
    if (lower == "exponential pdf" || lower == "5") return RADAR_RCS::ExponentialPDF;
    if (lower == "weibull pdf" || lower == "6") return RADAR_RCS::WeibullPDF;
    if (lower == "chisquared pdf" || lower == "7") return RADAR_RCS::ChiSquaredPDF;
    if (lower == "gamma pdf" || lower == "8") return RADAR_RCS::GammaPDF;
    if (lower == "beta pdf" || lower == "9") return RADAR_RCS::BetaPDF;
    if (lower == "f pdf" || lower == "10") return RADAR_RCS::FPDF;
    if (lower == "binomial cdf" || lower == "11") return RADAR_RCS::BinomialCDF;
    if (lower == "poisson cdf" || lower == "12") return RADAR_RCS::PoissonCDF;
    return RADAR_RCS::GaussianPDF;
}
