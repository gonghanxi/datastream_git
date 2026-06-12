#ifndef RADAR_PROPAGATIONLOSS_BLOCK_H
#define RADAR_PROPAGATIONLOSS_BLOCK_H

#include "Block.h"
#include "RADAR_PropagationLoss.h"

#include <memory>
#include <string>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_PropagationLoss_Block : public Block
{
public:
    RADAR_PropagationLoss_Block(const std::string& name);
    ~RADAR_PropagationLoss_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

private:
    void SetDefaultParameters();
    void SetParameters();
    bool validateAndPrepare();
    bool DataStreamRun();
    bool TimeDrivenRun();

    // 字符串 → 枚举转换
    static RADAR_PropagationLoss::OutputUnitEnum           ConvertStringToOutputUnit(const std::string& value);
    static RADAR_PropagationLoss::PropagationLossTypeEnum  ConvertStringToPropagationLossType(const std::string& value);
    static RADAR_PropagationLoss::RainLoss77GHzTypeEnum    ConvertStringToRainLoss77GHzType(const std::string& value);
    static RADAR_PropagationLoss::TempAntWtLyLossEnum      ConvertStringToTempAntWtLyLoss(const std::string& value);

    static double clamp(double x, double lo, double hi);

    // ---- algorithm instance ----
    std::unique_ptr<RADAR_PropagationLoss> m_algo;

    // ---- 参数 (统一使用原算法 public 枚举) ----
    RADAR_PropagationLoss::OutputUnitEnum           m_OutputUnit;
    RADAR_PropagationLoss::PropagationLossTypeEnum  m_PropagationLossType;
    RADAR_PropagationLoss::RainLoss77GHzTypeEnum    m_RainLoss77GHzType;

    double m_Frequency;
    double m_RainfallRate;
    double m_AntTheta;
    double m_AntPhi;
    double m_AntHeight;
    double m_Bandwidth;
    double m_TarRCS;

    RADAR_PropagationLoss::TempAntWtLyLossEnum m_TempAntWtLyLoss;
    double m_dw;

    double m_SnowfallRate;
};

RegAlgo(RADAR_PropagationLoss_Block);

#endif // RADAR_PROPAGATIONLOSS_BLOCK_H
