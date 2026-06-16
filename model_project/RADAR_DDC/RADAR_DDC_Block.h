#ifndef RADAR_DDC_BLOCK_H
#define RADAR_DDC_BLOCK_H
#include "RADAR_DDC.h"
#include "Block.h"
#include "EnvelopeSignal.h"
#include <queue>
#include <deque>
using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_DDC_Block : public Block
{
public:
    RADAR_DDC_Block(const std::string& name);
    ~RADAR_DDC_Block() = default;
    bool Setup() override;
    bool Initialize() override;
    bool Run() override;

    void SetParameters();
private:
    bool ModelSetup();
    void SetDefaultParameters();
    void UpdateCharacterizationFrequency();

    std::unique_ptr<RADAR_DDC> m_ddc;

    // 参数
    double IF_Freq;
    double IF_SamplingRate;
    int    ADC_NBits;
    double PhaseImbalance;
    int    BB_DownSamplingRatio;
    double RC_ExcessBW;
    double Out_CenterFreq;

    // 内部状态
    int decim_;
    double inputSampleRateHz_;
    double inputTimeStepSec_;
    long long m_totalSamplesProcessed;

    // FIR 滤波器状态
    std::vector<double> quadFir_;
    std::deque<std::complex<double>> quadFirState_;

    bool DataStreamRun();
    bool TimeDrivenRun();

    // ========== 时间驱动缓冲队列 ==========
    std::vector<EnvelopeSignal> m_inputBuffer;
    std::queue<std::complex<double>> m_outputQueue;
    std::complex<double> m_lastOutput;
    int m_inputCount;
    int m_outputCount;

    // ========== DDC 信号处理辅助函数 ==========
    void buildQuadSampleFir_();
    std::complex<double> runQuadSampleFir_(const std::complex<double>& x);

    double envelopeToRealIF_(const EnvelopeSignal& x,
        double inputFcHz, double timeNow) const;
    std::complex<double> envelopeToComplex_(const EnvelopeSignal& x,
        double fcHz) const;
    double applyADC_(double x) const;
    std::complex<double> quadSampleOneIFPoint_(double realIf,
        double timeNow) const;
    std::complex<double> applyOutCenterFreq_(const std::complex<double>& x,
        double timeNow) const;

    static double raisedCosineImpulse_(double t, double alpha);
    static double sinc_(double x);
    static double deg2rad(double x);
    static double clamp(double x, double lo, double hi);
};
RegAlgo(RADAR_DDC_Block);
#endif // RADAR_DDC_BLOCK_H
