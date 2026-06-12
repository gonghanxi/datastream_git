#include "RADAR_PropagationLoss_Block.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <string>

#ifndef M_PI
#define M_PI 3.141592653589793238462643383279502884
#endif

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

RADAR_PropagationLoss_Block::RADAR_PropagationLoss_Block(const std::string& name)
    : Block(name)
    , m_OutputUnit(RADAR_PropagationLoss::Output_dB)
    , m_PropagationLossType(RADAR_PropagationLoss::Rainfall)
    , m_RainLoss77GHzType(RADAR_PropagationLoss::Mid_Range_Loss)
    , m_Frequency(77e9)
    , m_RainfallRate(15.0)
    , m_AntTheta(3.0)
    , m_AntPhi(4.0)
    , m_AntHeight(0.5)
    , m_Bandwidth(500e6)
    , m_TarRCS(10.0)
    , m_TempAntWtLyLoss(RADAR_PropagationLoss::Temp_20)
    , m_dw(0.25e-3)
    , m_SnowfallRate(1.5)
{
}

void RADAR_PropagationLoss_Block::SetDefaultParameters()
{
    m_OutputUnit          = RADAR_PropagationLoss::Output_dB;
    m_PropagationLossType = RADAR_PropagationLoss::Rainfall;
    m_RainLoss77GHzType   = RADAR_PropagationLoss::Mid_Range_Loss;

    m_Frequency    = 77e9;
    m_RainfallRate = 15.0;
    m_AntTheta     = 3.0;
    m_AntPhi       = 4.0;
    m_AntHeight    = 0.5;
    m_Bandwidth    = 500e6;
    m_TarRCS       = 10.0;

    m_TempAntWtLyLoss = RADAR_PropagationLoss::Temp_20;
    m_dw              = 0.25e-3;

    m_SnowfallRate = 1.5;
}

void RADAR_PropagationLoss_Block::SetParameters()
{
    if (!m_algo) return;
    m_algo->OutputUnit          = m_OutputUnit;
    m_algo->PropagationLossType = m_PropagationLossType;
    m_algo->RainLoss77GHzType   = m_RainLoss77GHzType;
    m_algo->Frequency           = m_Frequency;
    m_algo->RainfallRate        = m_RainfallRate;
    m_algo->AntTheta            = m_AntTheta;
    m_algo->AntPhi              = m_AntPhi;
    m_algo->AntHeight           = m_AntHeight;
    m_algo->Bandwidth           = m_Bandwidth;
    m_algo->TarRCS              = m_TarRCS;
    m_algo->TempAntWtLyLoss     = m_TempAntWtLyLoss;
    m_algo->dw                  = m_dw;
    m_algo->SnowfallRate        = m_SnowfallRate;
}

// ============================================================================
// validateAndPrepare — 对齐原算法 Setup 校验逻辑
// ============================================================================

bool RADAR_PropagationLoss_Block::validateAndPrepare()
{
    if (m_Frequency <= 0.0)
    {
        LOG_ERROR("Frequency must be greater than 0.");
        return false;
    }

    if (m_PropagationLossType == RADAR_PropagationLoss::Rainfall)
    {
        if (m_RainfallRate <= 0.0)
        {
            LOG_ERROR("RainfallRate must be greater than 0.");
            return false;
        }
    }

    if (m_PropagationLossType == RADAR_PropagationLoss::Snowfall)
    {
        if (m_SnowfallRate <= 0.0)
        {
            LOG_ERROR("SnowfallRate must be greater than 0.");
            return false;
        }
    }

    if (m_PropagationLossType == RADAR_PropagationLoss::Rainfall_77GHz &&
        m_RainLoss77GHzType != RADAR_PropagationLoss::Antenna_Water_Layer_Loss)
    {
        if (m_RainfallRate <= 0.0)
        {
            LOG_ERROR("RainfallRate must be greater than 0.");
            return false;
        }
    }

    if (m_PropagationLossType == RADAR_PropagationLoss::Rainfall_77GHz &&
        m_RainLoss77GHzType == RADAR_PropagationLoss::Near_Range_Loss)
    {
        if (m_Bandwidth <= 0.0)
        {
            LOG_ERROR("Bandwidth must be greater than 0.");
            return false;
        }
        if (m_TarRCS <= 0.0)
        {
            LOG_ERROR("TarRCS must be greater than 0.");
            return false;
        }
        if (m_AntTheta <= 0.0)
        {
            LOG_ERROR("AntTheta must be greater than 0.");
            return false;
        }
        if (m_AntPhi <= 0.0)
        {
            LOG_ERROR("AntPhi must be greater than 0.");
            return false;
        }
    }

    if (m_PropagationLossType == RADAR_PropagationLoss::Rainfall_77GHz &&
        m_RainLoss77GHzType == RADAR_PropagationLoss::Antenna_Water_Layer_Loss)
    {
        if (m_dw < 0.0)
        {
            LOG_ERROR("dw must be greater than or equal to 0.");
            return false;
        }
    }

    return true;
}

bool RADAR_PropagationLoss_Block::Setup()
{
    Block::Setup();
    return true;
}

bool RADAR_PropagationLoss_Block::Run()
{
    return DataStreamRun();
}

bool RADAR_PropagationLoss_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_algo = std::make_unique<RADAR_PropagationLoss>();

    SetDefaultParameters();

    try { m_OutputUnit          = ConvertStringToOutputUnit(getParameter("OutputUnit").Value);                     } catch (...) { LOG_WARN("Failed to parse parameter 'OutputUnit', using default value."); }
    try { m_PropagationLossType = ConvertStringToPropagationLossType(getParameter("PropagationLossType").Value);   } catch (...) { LOG_WARN("Failed to parse parameter 'PropagationLossType', using default value."); }
    try { m_RainLoss77GHzType   = ConvertStringToRainLoss77GHzType(getParameter("RainLoss77GHzType").Value);       } catch (...) { LOG_WARN("Failed to parse parameter 'RainLoss77GHzType', using default value."); }
    try { m_Frequency           = std::stod(getParameter("Frequency").Value);                                      } catch (...) { LOG_WARN("Failed to parse parameter 'Frequency', using default value."); }
    try { m_RainfallRate        = std::stod(getParameter("RainfallRate").Value);                                   } catch (...) { LOG_WARN("Failed to parse parameter 'RainfallRate', using default value."); }
    try { m_AntTheta            = std::stod(getParameter("AntTheta").Value);                                       } catch (...) { LOG_WARN("Failed to parse parameter 'AntTheta', using default value."); }
    try { m_AntPhi              = std::stod(getParameter("AntPhi").Value);                                         } catch (...) { LOG_WARN("Failed to parse parameter 'AntPhi', using default value."); }
    try { m_AntHeight           = std::stod(getParameter("AntHeight").Value);                                      } catch (...) { LOG_WARN("Failed to parse parameter 'AntHeight', using default value."); }
    try { m_Bandwidth           = std::stod(getParameter("Bandwidth").Value);                                      } catch (...) { LOG_WARN("Failed to parse parameter 'Bandwidth', using default value."); }
    try { m_TarRCS              = std::stod(getParameter("TarRCS").Value);                                         } catch (...) { LOG_WARN("Failed to parse parameter 'TarRCS', using default value."); }
    try { m_TempAntWtLyLoss     = ConvertStringToTempAntWtLyLoss(getParameter("TempAntWtLyLoss").Value);           } catch (...) { LOG_WARN("Failed to parse parameter 'TempAntWtLyLoss', using default value."); }
    try { m_dw                  = std::stod(getParameter("dw").Value);                                             } catch (...) { LOG_WARN("Failed to parse parameter 'dw', using default value."); }
    try { m_SnowfallRate        = std::stod(getParameter("SnowfallRate").Value);                                   } catch (...) { LOG_WARN("Failed to parse parameter 'SnowfallRate', using default value."); }

    SetParameters();

    if (!validateAndPrepare()) {
        return false;
    }

    AddInputPort ("range",     m_algo->range,     1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("attenuate", m_algo->attenuate,  1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);

    return true;
}

// ============================================================================
// DataStreamRun — rate=1 单值直通
// ============================================================================

bool RADAR_PropagationLoss_Block::DataStreamRun()
{
    auto inputData = ReadInputData<double>(GetInputPortName(0));
    if (inputData.empty()) return true;

    const double r        = std::max(0.0, inputData[0]);
    const double lossDb   = m_algo->computeLossDb_(r);
    const double result   = m_algo->convertOutputUnit_(lossDb);

    WriteOutputData(GetOutputPortName(0), std::vector<double>{result});
    return true;
}

RADAR_PropagationLoss::OutputUnitEnum
RADAR_PropagationLoss_Block::ConvertStringToOutputUnit(const std::string& value)
{
    const std::string v = ToLowerCopy(TrimCopy(value));
    if (v == "0" || v == "db")                     return RADAR_PropagationLoss::Output_dB;
    if (v == "1" || v == "linear_power_loss" || v == "linear power loss")      return RADAR_PropagationLoss::Linear_Power_Loss;
    if (v == "2" || v == "linear_amplitude_loss" || v == "linear amplitude loss") return RADAR_PropagationLoss::Linear_Amplitude_Loss;

    try { return static_cast<RADAR_PropagationLoss::OutputUnitEnum>(std::stoi(value)); } catch (...) {}
    return RADAR_PropagationLoss::Output_dB;
}

RADAR_PropagationLoss::PropagationLossTypeEnum
RADAR_PropagationLoss_Block::ConvertStringToPropagationLossType(const std::string& value)
{
    const std::string v = ToLowerCopy(TrimCopy(value));
    if (v == "0" || v == "rainfall")                       return RADAR_PropagationLoss::Rainfall;
    if (v == "1" || v == "snowfall")                       return RADAR_PropagationLoss::Snowfall;
    if (v == "2" || v == "77ghz_rainfall" || v == "77ghz rainfall") return RADAR_PropagationLoss::Rainfall_77GHz;

    try { return static_cast<RADAR_PropagationLoss::PropagationLossTypeEnum>(std::stoi(value)); } catch (...) {}
    return RADAR_PropagationLoss::Rainfall;
}

RADAR_PropagationLoss::RainLoss77GHzTypeEnum
RADAR_PropagationLoss_Block::ConvertStringToRainLoss77GHzType(const std::string& value)
{
    const std::string v = ToLowerCopy(TrimCopy(value));
    if (v == "0" || v == "near_range_loss" || v == "near range loss")         return RADAR_PropagationLoss::Near_Range_Loss;
    if (v == "1" || v == "mid_range_loss" || v == "mid range loss")           return RADAR_PropagationLoss::Mid_Range_Loss;
    if (v == "2" || v == "antenna_water_layer_loss" || v == "antenna water layer loss") return RADAR_PropagationLoss::Antenna_Water_Layer_Loss;

    try { return static_cast<RADAR_PropagationLoss::RainLoss77GHzTypeEnum>(std::stoi(value)); } catch (...) {}
    return RADAR_PropagationLoss::Mid_Range_Loss;
}

RADAR_PropagationLoss::TempAntWtLyLossEnum
RADAR_PropagationLoss_Block::ConvertStringToTempAntWtLyLoss(const std::string& value)
{
    const std::string v = ToLowerCopy(TrimCopy(value));
    if (v == "0" || v == "-10" || v == "temp_minus10") return RADAR_PropagationLoss::Temp_minus10;
    if (v == "1" || v == "0"   || v == "temp_0")       return RADAR_PropagationLoss::Temp_0;
    if (v == "2" || v == "10"  || v == "temp_10")      return RADAR_PropagationLoss::Temp_10;
    if (v == "3" || v == "20"  || v == "temp_20")      return RADAR_PropagationLoss::Temp_20;
    if (v == "4" || v == "30"  || v == "temp_30")      return RADAR_PropagationLoss::Temp_30;
    if (v == "5" || v == "40"  || v == "temp_40")      return RADAR_PropagationLoss::Temp_40;
    if (v == "6" || v == "50"  || v == "temp_50")      return RADAR_PropagationLoss::Temp_50;

    try { return static_cast<RADAR_PropagationLoss::TempAntWtLyLossEnum>(std::stoi(value)); } catch (...) {}
    return RADAR_PropagationLoss::Temp_20;
}

// ============================================================================
// clamp — 原算法的静态工具函数
// ============================================================================

double RADAR_PropagationLoss_Block::clamp(double x, double lo, double hi)
{
    if (x != x) return lo;
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}
