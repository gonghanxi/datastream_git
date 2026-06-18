#ifndef ATOD_BLOCK_H
#define ATOD_BLOCK_H

#include "Block.h"
#include "AtoD.h"

#include <queue>
#include <random>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API AtoD_Block : public SystemVueModelBuilder::Block
{
public:
    AtoD_Block(const std::string& name);
    ~AtoD_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

private:
    // 量化结果结构
    struct QuantResult {
        int codeOffset;
        int codeDigital;
        double analog;
    };

    bool DataStreamRun();
    bool TimeDrivenRun();
    void SetDefaultParamters();
    void SetParameters();
    bool parseArrayString(const std::string& arrayStr, std::vector<double>& outArray);

    AtoD::OutputDigitalFormatEnum ConvertStringToOutputDigitalFormatEnum(const std::string& value);
    AtoD::DistortionModelEnum ConvertStringToDistortionModelEnum(const std::string& value);
    AtoD::EnableJitterEnum ConvertStringToEnableJitterEnum(const std::string& value);
    AtoD::PN_TypeEnum ConvertStringToPN_TypeEnum(const std::string& value);
    AtoD::FFT_SizeEnum ConvertStringToFFT_SizeEnum(const std::string& value);
    AtoD::SNR_ModelEnum ConvertStringToSNR_ModelEnum(const std::string& value);
    AtoD::ConversionTypeEnum ConvertStringToConversionTypeEnum(const std::string& value);
    AtoD::AntiAliasingFilterEnum ConvertStringToAntiAliasingFilterEnum(const std::string& value);
    AtoD::ADCTypeEnum ConvertStringToADCTypeEnum(const std::string& value);

    // 核心算法方法
    void initQuantizationParams();
    QuantResult quantize(double x) const;
    QuantResult quantizeFlash(double x) const;
    std::complex<double> processPipeline(const std::complex<double>& x);
    std::complex<double> processSigmaDelta(const std::complex<double>& x);

    // Clocked 模式采样保持
    std::complex<double> getClockInput(const std::complex<double>& x, double t);
    double firstPositiveCrossingAtOrAfter(double t) const;
    std::complex<double> interpSample(const std::complex<double>& x0, double t0,
                                      const std::complex<double>& x1, double t1, double ts) const;

    std::unique_ptr<AtoD> m_atod;

    // --------- 参数 ---------
    int    m_NBits;
    double m_VRef;

    AtoD::ADCTypeEnum m_ADCType;

    AtoD::OutputDigitalFormatEnum m_OutputDigitalFormat;
    AtoD::DistortionModelEnum     m_DistortionModel;

    AtoD::EnableJitterEnum m_EnableJitter;
    double m_RJrms;

    double* m_PhaseNoiseData;
    int     m_PhaseNoiseDataSize;
    AtoD::PN_TypeEnum m_PN_Type;

    double m_INL;
    double m_DNL;

    double m_ENOB;
    double m_SNR_dB;
    double m_H2_dBc;
    double m_H3_dBc;
    double m_H4_dBc;
    double m_H5_dBc;

    double m_SINAD_dB;
    double m_SFDR_dBc;
    AtoD::FFT_SizeEnum m_FFT_Size;

    AtoD::SNR_ModelEnum m_SNR_Model;
    double m_ThermalNoise_SNR_dBFS;
    double m_CenterFreq;
    double m_Level_dBFS;

    AtoD::ConversionTypeEnum m_ConversionType;
    double m_Clock;
    double m_Phase;

    int m_DownsampleFactor;
    int m_DownsamplePhase;
    AtoD::AntiAliasingFilterEnum m_AntiAliasingFilter;
    double m_ExcessBW;

    int m_PipelineStageBits;
    int m_PipelineLatency;
    int m_SigmaDeltaOrder;
    int m_SigmaDeltaOSR;

    std::vector<double> primdata;
    double m_sampleRate;

    // ===== TimeDrivenRun 缓存与队列 =====
    std::vector<SystemVueModelBuilder::EnvelopeSignal> m_inputBuffer;
    std::queue<SystemVueModelBuilder::EnvelopeSignal> m_aoutQueue;
    std::queue<double> m_diQueue;
    std::queue<double> m_dqQueue;
    int m_inputRate = 1;

    // ===== 量化相关状态变量 =====
    int m_nbits;
    int m_codeCount;
    int m_midCode;
    double m_vref;
    double m_lsb;
    unsigned long long m_sampleIndex;

    // Pipeline ADC 状态
    std::vector<std::complex<double>> m_pipelineFifo;

    // Sigma-Delta ADC 状态
    double m_sdIIntegrator;
    double m_sdQIntegrator;
    double m_sdIFeedback;
    double m_sdQFeedback;
    double m_sdIAccum;
    double m_sdQAccum;
    int m_sdAccumCount;
    std::complex<double> m_sdHeldOutput;

    // Clocked 模式采样保持状态
    bool m_hasClockState;
    double m_lastClockValue;
    std::complex<double> m_heldSample;
    bool m_hasPendingClockSample;
    std::complex<double> m_pendingClockSample;
    bool m_hasRawInputState;
    double m_prevRawInputTime;
    std::complex<double> m_prevRawInput;
    bool m_hasNextClockCrossing;
    double m_nextClockCrossingTime;

    // 随机数生成器
    std::mt19937 m_rng;

    SimuParameter simulator_param;
};

RegAlgo(AtoD_Block);
#endif // ATOD_BLOCK_H
