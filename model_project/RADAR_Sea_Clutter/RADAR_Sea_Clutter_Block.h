#ifndef RADAR_SEA_CLUTTER_BLOCK_H
#define RADAR_SEA_CLUTTER_BLOCK_H

#include "Block.h"
#include "RADAR_Sea_Clutter.h"

#include <complex>
#include <memory>
#include <queue>
#include <random>
#include <string>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_Sea_Clutter_Block : public SystemVueModelBuilder::Block
{
public:
    RADAR_Sea_Clutter_Block(const std::string& name);
    ~RADAR_Sea_Clutter_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

    void SetDefaultParameters();
    void SetParameters();

private:
    bool DataStreamRun();
    bool TimeDrivenRun();

    // ConvertStringTo
    static RADAR_Sea_Clutter::SelectedSeaState       ConvertStringToSeaState(const std::string& value);
    static RADAR_Sea_Clutter::SelectedAntenna_Pattern ConvertStringToAntennaPattern(const std::string& value);

    // 算法辅助函数
    void computeSigma();
    void generateClutter(int numSample);

    std::unique_ptr<RADAR_Sea_Clutter> m_algo;

    // ===== 参数 =====
    RADAR_Sea_Clutter::SelectedSeaState       m_SeaState;       // 海况
    double m_RF_Freq;
    RADAR_Sea_Clutter::SelectedAntenna_Pattern m_AntennaPattern; // 天线方向图
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
    double m_SS;               // 海况数值 (1~7)
    double m_WindVelocity;
    double m_Sigma;

    // ===== 缓存与状态 =====
    std::vector<std::complex<double>> m_clutter; // 生成的杂波序列
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

RegAlgo(RADAR_Sea_Clutter_Block);

#endif // RADAR_SEA_CLUTTER_BLOCK_H
