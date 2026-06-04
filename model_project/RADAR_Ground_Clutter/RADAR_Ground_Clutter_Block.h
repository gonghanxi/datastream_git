#ifndef RADAR_GROUND_CLUTTER_BLOCK_H
#define RADAR_GROUND_CLUTTER_BLOCK_H

#include "Block.h"
#include "RADAR_Ground_Clutter.h"

#include <complex>
#include <memory>
#include <queue>
#include <random>
#include <string>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_Ground_Clutter_Block : public SystemVueModelBuilder::Block
{
public:
    RADAR_Ground_Clutter_Block(const std::string& name);
    ~RADAR_Ground_Clutter_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

    void SetDefaultParameters();
    void SetParameters();

private:
    bool DataStreamRun();
    bool TimeDrivenRun();

    // ConvertStringTo
    static RADAR_Ground_Clutter::SelectedGroundType      ConvertStringToGroundType(const std::string& value);
    static RADAR_Ground_Clutter::SelectedAntenna_Pattern  ConvertStringToAntennaPattern(const std::string& value);

    // 算法辅助函数
    void computeSigma();
    void generateClutter(int numSample);

    std::unique_ptr<RADAR_Ground_Clutter> m_algo;

    // ===== 参数 =====
    RADAR_Ground_Clutter::SelectedGroundType       m_GroundType;
    double m_RF_Freq;
    RADAR_Ground_Clutter::SelectedAntenna_Pattern  m_AntennaPattern;
    double m_Scatter0;
    double m_GrazingAngle;
    double m_BodyRollAngle;
    double m_BodyPitchAngle;
    double m_BodyYawAngle;
    double m_AntTiltAngle;
    double m_AntYawAngle;
    double m_PRF;
    double m_SampleRate;
    double m_AntennaHeight;
    double m_PlatformVelocity;

    // ===== 算法内部状态 =====
    double m_Sigma;

    // ===== 缓存与状态 =====
    std::vector<std::complex<double>> m_clutter;
    int m_cachedNumSample;
    std::mt19937 m_rng;

    // ===== TimeDrivenRun 逐点累积 =====
    std::vector<EnvelopeSignal> m_inputBuffer;
    std::queue<EnvelopeSignal>  m_outputQueue;
    std::queue<EnvelopeSignal>  m_clutterQueue;

    // ===== 仿真参数 =====
    SimuParameter simulator_param;

    static constexpr double kPi = 3.14159265358979323846;
};

RegAlgo(RADAR_Ground_Clutter_Block);

#endif // RADAR_GROUND_CLUTTER_BLOCK_H
