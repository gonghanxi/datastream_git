#ifndef RADAR_SAR_ECHO_BLOCK_H
#define RADAR_SAR_ECHO_BLOCK_H

#include "Block.h"
#include "RADAR_SAR_Echo.h"

#include <queue>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_SAR_Echo_Block : public SystemVueModelBuilder::Block
{
public:
    RADAR_SAR_Echo_Block(const std::string& name);
    ~RADAR_SAR_Echo_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

private:
    bool DataStreamRun();
    bool TimeDrivenRun();
    void SetDefaultParamters();
    void SetParameters();

    RADAR_SAR_Echo::SelectedSAR_Mode ConvertStringToSAR_Mode(const std::string& value);
    RADAR_SAR_Echo::SelectedEchoGenerate_Mode ConvertStringToEchoGenerate_Mode(const std::string& value);

    std::unique_ptr<RADAR_SAR_Echo> m_RADAR_SAR_Echo;

    // Parameter
    RADAR_SAR_Echo::SelectedSAR_Mode m_SAR_Mode;
    double m_Fc;
    double m_Xmin;
    double m_Xmax;
    double m_Yc;
    double m_Y0;
    double m_H;
    double m_Vr;
    double m_D;
    double m_Tr;
    double m_Br;
    double m_SampleRate;
    RADAR_SAR_Echo::SelectedEchoGenerate_Mode m_EchoGenerate_Mode;
    SystemVueModelBuilder::Matrix<double> m_TargetInfo;

    // ===== TimeDrivenRun 逐点输出 =====
    std::queue<std::complex<double>> m_outputQueue;
    bool m_dataGenerated = false;

    SimuParameter simulator_param;
};

RegAlgo(RADAR_SAR_Echo_Block);

#endif // RADAR_SAR_ECHO_BLOCK_H
