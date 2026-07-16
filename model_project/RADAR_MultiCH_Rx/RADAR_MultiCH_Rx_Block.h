#ifndef RADAR_MULTICH_RX_BLOCK_H
#define RADAR_MULTICH_RX_BLOCK_H

#include "Block.h"
#include "RADAR_MultiCH_Rx.h"

#include <complex>
#include <memory>
#include <queue>
#include <random>
#include <string>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_MultiCH_Rx_Block : public SystemVueModelBuilder::Block
{
public:
    RADAR_MultiCH_Rx_Block(const std::string& name);
    ~RADAR_MultiCH_Rx_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;
    int  GetBusChannelCount() const override { return m_NumOfCh; }

    void SetDefaultParameters();
    void SetParameters();

private:
    bool DataStreamRun();
    bool TimeDrivenRun();

    // 解析参数数组字符串
    static bool parseDoubleArray(const std::string& str, std::vector<double>& out);
    static bool parseComplexArray(const std::string& str, std::vector<std::complex<double>>& out);

    // 算法辅助函数
    void rebuildCache();
    static double deg2rad(double deg);
    static std::complex<double> applyIQImbalance(const std::complex<double>& z, double gainDb, double phaseDeg);
    std::complex<double> makeNoise(double fs);

    std::unique_ptr<RADAR_MultiCH_Rx> m_algo;

    // ===== 参数 =====
    double m_RefFreq;
    double m_NDensity;
    int    m_NumOfCh;

    // ===== 缓存 =====
    int m_nChExpected;
    std::vector<double> m_sens;
    std::vector<double> m_phaseDeg;
    std::vector<double> m_iqGainDb;
    std::vector<double> m_iqPhaseDeg;
    std::vector<std::complex<double>> m_imbCoef;

    // ===== 噪声状态 =====
    std::mt19937 m_rng;
    bool m_haveSpare;
    double m_spare;

    // ===== 采样计数器 =====
    uint64_t m_sampleCount;

    // ===== 仿真参数 =====
    SimuParameter simulator_param;

    // ===== TimeDrivenRun 逐点累积 =====
    std::vector<EnvelopeSignal> m_inputBuffer;
    std::queue<std::complex<double>> m_outputQueue;

    static constexpr double kPi    = 3.14159265358979323846;
    static constexpr double kTwoPi = 6.28318530717958647692;
};

RegAlgo(RADAR_MultiCH_Rx_Block);

#endif // RADAR_MULTICH_RX_BLOCK_H
