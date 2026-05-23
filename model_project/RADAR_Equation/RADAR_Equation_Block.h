#ifndef RADAR_EQUATION_BLOCK_H
#define RADAR_EQUATION_BLOCK_H

#include "RADAR_Equation.h"
#include "Block.h"

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_Equation_Block : public SystemVueModelBuilder::Block
{
public:
    RADAR_Equation_Block(const std::string& name);
    ~RADAR_Equation_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();

private:
    RADAR_Equation::SelectedEqType ConvertStringToSelectedEqType(const std::string& value);
    RADAR_Equation::SelectedOutputType ConvertStringToSelectedOutputType(const std::string& value);
    RADAR_Equation::SelectedAntennaType ConvertStringToSelectedAntennaType(const std::string& value);
    RADAR_Equation::SelectedIntegrationType ConvertStringToSelectedIntegrationType(const std::string& value);

    void SetDefaultParameters();

    std::unique_ptr<RADAR_Equation> m_radarEquation;

    RADAR_Equation::SelectedEqType m_EqType;
    RADAR_Equation::SelectedOutputType m_OutputType;
    double m_Pt;
    double m_Pavg;
    double m_DwellTime;
    double m_PRF;
    RADAR_Equation::SelectedAntennaType m_AntennaType;
    double m_Gain;
    double m_GainTx;
    double m_GainRx;
    double m_RCS;
    double m_NoiseFigure;
    double m_SystemNoiseTemperature;
    double m_Freq;
    double m_Pulsewidth;
    double m_Bandwidth;
    double m_SystemLoss;
    double m_PropagationLoss;
    double m_GroundPlaneLoss;
    double m_Range;
    double m_SNR;
    RADAR_Equation::SelectedIntegrationType m_IntegrationType;
    double m_PulseNumber;
    double m_IntegrationLoss;
    double m_Theta3dB;
    double m_ScanRate;
    double m_ServoBandwidth;
};

RegAlgo(RADAR_Equation_Block);

#endif // RADAR_EQUATION_BLOCK_H
