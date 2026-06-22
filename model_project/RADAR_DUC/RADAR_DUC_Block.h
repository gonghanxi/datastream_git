#ifndef RADAR_DUC_BLOCK_H
#define RADAR_DUC_BLOCK_H

#include "RADAR_DUC.h"
#include "Block.h"
#include "EnvelopeSignal.h"
#include <memory>
#include <queue>
#include <deque>
#include <vector>
#include <complex>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_DUC_Block : public Block
{
public:
    RADAR_DUC_Block(const std::string& name);
    ~RADAR_DUC_Block() = default;

    bool Setup() override;
    bool Initialize() override;
    bool Run() override;

    void SetParameters();

private:
    bool ModelSetup();
    void SetDefaultParameters();
    void UpdateCharacterizationFrequency();

    bool DataStreamRun();
    bool TimeDrivenRun();

    std::unique_ptr<RADAR_DUC> m_duc;

    // ========== 参数 ==========
    double IF_Freq;
    double IF_SamplingRate;
    double BandWidth;
    double In_CenterFreq;
    int    BB_UpSamplingRatio;
    double RC_ExcessBW;
    double PhaseImbalance;
    int    DAC_NBits;

    // ========== 运行时状态 ==========
    int upRate_;
    int outRate_;
    double outputSampleRateHz_;
    double outputTimeStepSec_;
    long long m_totalSamplesProcessed;

    // ========== FIR 滤波器状态 ==========
    std::vector<double> ducFir_;
    std::deque<std::complex<double>> ducFirState_;

    // ========== IF BPF 双二阶滤波器状态 ==========
    struct BiquadState
    {
        double b0, b1, b2, a1, a2;
        std::complex<double> x1, x2, y1, y2;
        void reset();
    };

    bool ifBpfEnabled_;
    BiquadState ifBpfSec1_;
    BiquadState ifBpfSec2_;

    // ========== 时间驱动缓冲队列 ==========
    std::queue<std::complex<double>> m_inputQueue;
    std::queue<EnvelopeSignal> m_outputQueue;
    int m_outputCount;

    // ========== DUC 信号处理辅助函数 ==========
    void buildRaisedCosineFir_();
    std::complex<double> runDucInterpolationFir_(const std::complex<double>& x);
    void configureIfBpf_();
    std::complex<double> runBiquad_(const std::complex<double>& x, BiquadState& s);
    std::complex<double> runIfBpf_(const std::complex<double>& x);
    std::complex<double> applyInputCenterFrequency_(const std::complex<double>& x, double timeNow) const;
    std::complex<double> applyDUCToIFEnvelope_(const std::complex<double>& x, double timeNow) const;
    std::complex<double> applyFcChangeImage_(const std::complex<double>& idealEnvelope, double timeNow) const;
    std::complex<double> applyFinalComplexConvention_(const std::complex<double>& x, double timeNow) const;
    double applyDAC_(double x) const;

    static double raisedCosineImpulse_(double t, double alpha);
    static double sinc_(double x);
    static double deg2rad(double x);
    static double clamp(double x, double lo, double hi);
};

RegAlgo(RADAR_DUC_Block);

#endif // RADAR_DUC_BLOCK_H
