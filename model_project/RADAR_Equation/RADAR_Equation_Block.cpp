#include "RADAR_Equation_Block.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

RADAR_Equation_Block::RADAR_Equation_Block(const std::string& name)
    : Block(name)
{
}

bool RADAR_Equation_Block::Setup()
{
    Block::Setup();
    return true;
}

static std::string TrimCopy(const std::string& value)
{
    std::string s = value;
    s.erase(0, s.find_first_not_of(" \t\n\r"));
    s.erase(s.find_last_not_of(" \t\n\r") + 1);
    return s;
}

static std::string ToLowerCopy(const std::string& value)
{
    std::string s = value;
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

RADAR_Equation::SelectedEqType RADAR_Equation_Block::ConvertStringToSelectedEqType(const std::string& value)
{
    const std::string lowerValue = ToLowerCopy(TrimCopy(value));
    if (lowerValue == "basic" || lowerValue == "0") {
        return RADAR_Equation::Basic;
    }
    if (lowerValue == "cw" || lowerValue == "1") {
        return RADAR_Equation::CW;
    }
    if (lowerValue == "pd" || lowerValue == "2") {
        return RADAR_Equation::PD;
    }
    if (lowerValue == "search" || lowerValue == "3") {
        return RADAR_Equation::Search;
    }
    if (lowerValue == "track" || lowerValue == "4") {
        return RADAR_Equation::Track;
    }
    return RADAR_Equation::Basic;
}

RADAR_Equation::SelectedOutputType RADAR_Equation_Block::ConvertStringToSelectedOutputType(const std::string& value)
{
    const std::string lowerValue = ToLowerCopy(TrimCopy(value));
    if (lowerValue == "snrout" || lowerValue == "snr" || lowerValue == "0") {
        return RADAR_Equation::SNROut;
    }
    if (lowerValue == "rangeout" || lowerValue == "maximum range" || lowerValue == "1") {
        return RADAR_Equation::RangeOut;
    }
    return RADAR_Equation::SNROut;
}

RADAR_Equation::SelectedAntennaType RADAR_Equation_Block::ConvertStringToSelectedAntennaType(const std::string& value)
{
    const std::string lowerValue = ToLowerCopy(TrimCopy(value));
    if (lowerValue == "single" || lowerValue == "0") {
        return RADAR_Equation::Single;
    }
    if (lowerValue == "monostaticseparate" || lowerValue == "monostatic separate" || lowerValue == "1") {
        return RADAR_Equation::MonostaticSeparate;
    }
    return RADAR_Equation::Single;
}

RADAR_Equation::SelectedIntegrationType RADAR_Equation_Block::ConvertStringToSelectedIntegrationType(const std::string& value)
{
    const std::string lowerValue = ToLowerCopy(TrimCopy(value));
    if (lowerValue == "singlehit" || lowerValue == "single hit" || lowerValue == "0") {
        return RADAR_Equation::Singlehit;
    }
    if (lowerValue == "integration" || lowerValue == "1") {
        return RADAR_Equation::Integration;
    }
    return RADAR_Equation::Singlehit;
}

void RADAR_Equation_Block::SetDefaultParameters()
{
    m_EqType = RADAR_Equation::Basic;
    m_OutputType = RADAR_Equation::SNROut;
    m_Pt = 1e6;
    m_Pavg = 1e6;
    m_DwellTime = 100e-6;
    m_PRF = 100e3;
    m_AntennaType = RADAR_Equation::Single;
    m_Gain = 30.0;
    m_GainTx = 30.0;
    m_GainRx = 30.0;
    m_RCS = 1.0;
    m_NoiseFigure = 2.0;
    m_SystemNoiseTemperature = 16.85;
    m_Freq = 10e9;
    m_Pulsewidth = 1e-6;
    m_Bandwidth = 5e6;
    m_SystemLoss = 4.0;
    m_PropagationLoss = 4.0;
    m_GroundPlaneLoss = 0.0;
    m_Range = 100e3;
    m_SNR = 10.0;
    m_IntegrationType = RADAR_Equation::Singlehit;
    m_PulseNumber = 1.0;
    m_IntegrationLoss = 0.0;
    m_Theta3dB = 3.0;
    m_ScanRate = 40.0;
    m_ServoBandwidth = 5.0;
}

void RADAR_Equation_Block::SetParameters()
{
    if (!m_radarEquation) {
        return;
    }

    m_radarEquation->EqType = m_EqType;
    m_radarEquation->OutputType = m_OutputType;
    m_radarEquation->Pt = m_Pt;
    m_radarEquation->Pavg = m_Pavg;
    m_radarEquation->DwellTime = m_DwellTime;
    m_radarEquation->PRF = m_PRF;
    m_radarEquation->AntennaType = m_AntennaType;
    m_radarEquation->Gain = m_Gain;
    m_radarEquation->GainTx = m_GainTx;
    m_radarEquation->GainRx = m_GainRx;
    m_radarEquation->RCS = m_RCS;
    m_radarEquation->NoiseFigure = m_NoiseFigure;
    m_radarEquation->SystemNoiseTemperature = m_SystemNoiseTemperature;
    m_radarEquation->Freq = m_Freq;
    m_radarEquation->Pulsewidth = m_Pulsewidth;
    m_radarEquation->Bandwidth = m_Bandwidth;
    m_radarEquation->SystemLoss = m_SystemLoss;
    m_radarEquation->PropagationLoss = m_PropagationLoss;
    m_radarEquation->GroundPlaneLoss = m_GroundPlaneLoss;
    m_radarEquation->Range = m_Range;
    m_radarEquation->SNR = m_SNR;
    m_radarEquation->IntegrationType = m_IntegrationType;
    m_radarEquation->PulseNumber = m_PulseNumber;
    m_radarEquation->IntegrationLoss = m_IntegrationLoss;
    m_radarEquation->Theta3dB = m_Theta3dB;
    m_radarEquation->ScanRate = m_ScanRate;
    m_radarEquation->ServoBandwidth = m_ServoBandwidth;
}

bool RADAR_Equation_Block::Initialize()
{
    SetBlockType(Block::BlockType::SOURCE);

    m_radarEquation = std::make_unique<RADAR_Equation>();

    AddOutputPort("output", m_radarEquation->output, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);

    SetDefaultParameters();

    auto readDoubleParam = [&](const std::string& name, double& outValue) {
        try {
            outValue = std::stod(getParameter(name).Value);
        } catch (...) {
            LOG_WARN("Failed to parse parameter '%s', using default value.", name.c_str());
        }
    };

    auto readEqType = [&](const std::string& name, RADAR_Equation::SelectedEqType& outValue) {
        try {
            outValue = ConvertStringToSelectedEqType(getParameter(name).Value);
        } catch (...) {
            LOG_WARN("Failed to parse parameter '%s', using default value.", name.c_str());
        }
    };

    auto readOutputType = [&](const std::string& name, RADAR_Equation::SelectedOutputType& outValue) {
        try {
            outValue = ConvertStringToSelectedOutputType(getParameter(name).Value);
        } catch (...) {
            LOG_WARN("Failed to parse parameter '%s', using default value.", name.c_str());
        }
    };

    auto readAntennaType = [&](const std::string& name, RADAR_Equation::SelectedAntennaType& outValue) {
        try {
            outValue = ConvertStringToSelectedAntennaType(getParameter(name).Value);
        } catch (...) {
            LOG_WARN("Failed to parse parameter '%s', using default value.", name.c_str());
        }
    };

    auto readIntegrationType = [&](const std::string& name, RADAR_Equation::SelectedIntegrationType& outValue) {
        try {
            outValue = ConvertStringToSelectedIntegrationType(getParameter(name).Value);
        } catch (...) {
            LOG_WARN("Failed to parse parameter '%s', using default value.", name.c_str());
        }
    };

    readEqType("EqType", m_EqType);
    readOutputType("OutputType", m_OutputType);
    readDoubleParam("Pt", m_Pt);
    readDoubleParam("Pavg", m_Pavg);
    readDoubleParam("DwellTime", m_DwellTime);
    readDoubleParam("PRF", m_PRF);
    readAntennaType("AntennaType", m_AntennaType);
    readDoubleParam("Gain", m_Gain);
    readDoubleParam("GainTx", m_GainTx);
    readDoubleParam("GainRx", m_GainRx);
    readDoubleParam("RCS", m_RCS);
    readDoubleParam("NoiseFigure", m_NoiseFigure);
    readDoubleParam("SystemNoiseTemperature", m_SystemNoiseTemperature);
    readDoubleParam("Freq", m_Freq);
    readDoubleParam("Pulsewidth", m_Pulsewidth);
    readDoubleParam("Bandwidth", m_Bandwidth);
    readDoubleParam("SystemLoss", m_SystemLoss);
    readDoubleParam("PropagationLoss", m_PropagationLoss);
    readDoubleParam("GroundPlaneLoss", m_GroundPlaneLoss);
    readDoubleParam("Range", m_Range);
    readDoubleParam("SNR", m_SNR);
    readIntegrationType("IntegrationType", m_IntegrationType);
    readDoubleParam("PulseNumber", m_PulseNumber);
    readDoubleParam("IntegrationLoss", m_IntegrationLoss);
    readDoubleParam("Theta3dB", m_Theta3dB);
    readDoubleParam("ScanRate", m_ScanRate);
    readDoubleParam("ServoBandwidth", m_ServoBandwidth);

    SetParameters();

    return true;
}

bool RADAR_Equation_Block::Run()
{
    if (!CanProcess()) {
        return false;
    }

    if (!m_radarEquation) {
        return false;
    }

    if (!m_radarEquation->Run()) {
        return false;
    }

    std::string outputPortName = GetOutputPortName(0);
    std::vector<double> outputData;
    outputData.push_back(m_radarEquation->output[0U]);
    WriteOutputData(outputPortName, outputData);

    return true;
}
