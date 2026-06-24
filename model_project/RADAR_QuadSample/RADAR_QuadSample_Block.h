#ifndef RADAR_QUADSAMPLE_BLOCK_H
#define RADAR_QUADSAMPLE_BLOCK_H

#include "RADAR_QuadSample.h"
#include "Block.h"
#include <memory>
#include <queue>
#include <deque>
#include <vector>
#include <complex>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_QuadSample_Block : public Block
{
public:
    RADAR_QuadSample_Block(const std::string& name);
    ~RADAR_QuadSample_Block() = default;

    bool Setup() override;
    bool Initialize() override;
    bool Run() override;

    void SetParameters();

private:
    bool ModelSetup();
    void SetDefaultParameters();

    bool DataStreamRun();
    bool TimeDrivenRun();

    std::unique_ptr<RADAR_QuadSample> m_qs;

    // ========== 参数 ==========
    int    BB_DownSamplingRatio;
    double IF_Freq;
    double IF_SamplingRate;
    double Out_CenterFreq;
    double PhaseImbalance;
    double RC_ExcessBW;

    // ========== 运行时状态 ==========
    int decim_;
    double inputSampleRateHz_;
    double outputSampleRateHz_;
    double inputTimeStepSec_;
    unsigned long long m_count;

    // ========== FIR 滤波器状态 ==========
    std::vector<double> quadFir_;
    std::deque<std::complex<double>> quadFirState_;

    // ========== 时间驱动缓冲队列 ==========
    std::deque<double> m_inputBuffer;
    std::queue<std::complex<double>> m_outputQueue;

    // ========== 算法实现 ==========
    void buildQuadSampleFir_();
    std::complex<double> runQuadSampleFir_(const std::complex<double>& x);
    std::complex<double> quadSampleOneIFPoint_(double realIf, double timeNow) const;
    std::complex<double> applyOutCenterFreq_(const std::complex<double>& x, double timeNow) const;

    static double raisedCosineImpulse_(double t, double alpha);
    static double sinc_(double x);
    static double deg2rad(double x);
    static double clamp(double x, double lo, double hi);
};

RegAlgo(RADAR_QuadSample_Block);

#endif // RADAR_QUADSAMPLE_BLOCK_H
