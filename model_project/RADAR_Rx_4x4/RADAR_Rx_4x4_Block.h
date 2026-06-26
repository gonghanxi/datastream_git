#ifndef RADAR_RX_4X4_BLOCK_H
#define RADAR_RX_4X4_BLOCK_H

#include "RADAR_Rx_4x4.h"
#include "Block.h"
#include "EnvelopeSignal.h"

#include <complex>
#include <deque>
#include <memory>
#include <string>
#include <vector>
#include <cstdint>
#include <queue>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_Rx_4x4_Block : public Block
{
public:
    RADAR_Rx_4x4_Block(const std::string& name);
    ~RADAR_Rx_4x4_Block() = default;

    bool Setup() override;
    bool Initialize() override;
    bool Run() override;

    void SetDefaultParameters();
    void SetParameters();

private:
    bool ModelSetup();
    void initFromBusConnections_();
    bool DataStreamRun();
    bool TimeDrivenRun();

    // 仅用于端口注册的算法实例
    std::unique_ptr<RADAR_Rx_4x4> m_algo;

    // ========== 枚举 ==========
    enum SelectedGCType {
        none = 0, TOI = 1, dBc1 = 2, TOI_dBc1 = 3,
        PSat_GCSat_TOI = 4, PSat_GCSat_dBc1 = 5,
        PSat_GCSat_TOI_dBc1 = 6, RappNonlinearity = 7,
        Gain_compression_vs_input_power = 8,
        AM_AM_and_AMPM_vs_input_power = 9
    };

    using Cx = std::complex<double>;

    // ========== 参数 ==========
    double TStep_;
    double RF_Freq_;
    Cx RF_Gain_;
    double IF_Freq_;
    Cx IF_Gain_;
    double IF_SamplingRate_;
    double BandWidth_;
    int ADC_NBits_;
    double PhaseImbalance_;
    int BB_DownSamplingRatio_;
    double RC_ExcessBW_;
    double Out_CenterFreq_;
    double NoiseFigure_RFGain_;
    double NoiseFigure_IFGain_;
    double NoiseFigure_Mixer_;

    SelectedGCType GCType_RFGain_;
    double TOIout_RFGain_;
    double dBc1out_RFGain_;
    double PSat_RFGain_;
    double GCSat_RFGain_;
    std::vector<double> GComp_RFGain_Vec_;

    SelectedGCType GCType_IFGain_;
    double TOIout_IFGain_;
    double dBc1out_IFGain_;
    double PSat_IFGain_;
    double GCSat_IFGain_;
    std::vector<double> GComp_IFGain_Vec_;

    int NumRxAnt_;
    double ChannelDelay_;

    // ========== 运行时状态 ==========
    size_t inBusSize_;
    size_t outBusSize_;
    size_t activeChannels_;
    double sampleRateHz_;
    double timeStepSec_;
    int decim_;
    int delaySamples_;
    bool noisePrepared_;
    double noiseSigmaRF_;
    double noiseSigmaIF_;
    double noiseSigmaMixer_;
    bool bpfEnabled_;
    bool useLowFreqStartupCorrection_;
    unsigned long long firingCount_;
    bool busStateInitialized_;

    // ========== 内部结构 ==========
    struct GCompTable {
        bool valid;
        std::vector<double> pinDbm;
        std::vector<double> gainChangeDb;
        std::vector<double> phaseChangeDeg;
        GCompTable() : valid(false) {}
    };

    struct BiquadState {
        double b0, b1, b2, a1, a2;
        Cx x1, x2, y1, y2;
        BiquadState();
        void reset();
    };

    struct ChannelState {
        double inputFcHz;
        BiquadState bpfSec1, bpfSec2, bpfSec3, bpfSec4;
        std::deque<Cx> delayLine;
        uint32_t seedRF, seedIF, seedMixer;
        long outputCount;
        ChannelState();
        void reset();
    };

    std::vector<ChannelState> ch_;
    GCompTable rfTable_;
    GCompTable ifTable_;

    // ========== 时间驱动模式缓冲区 ==========
    struct TDRFrame {
        std::vector<EnvelopeSignal> data;
        std::vector<double> fcPerLane;
        double timeNow;
    };
    std::deque<TDRFrame> m_inputBuffer;
    std::queue<std::vector<Cx>> m_outputQueue;

    // ========== 初始化辅助 ==========
    bool prepareTables_();
    bool prepareNoise_();
    bool parseGCompArray_(const std::vector<double>& data, GCompTable& table) const;
    void configureBpfFilter_();
    void resetChannelStates_();
    void applyInputRates_();
    void applyOutputTiming_();
    int computeDelaySamples_() const;
    bool isLowFreqStartupCorrectionCase_() const;

    // ========== 信号处理 ==========
    Cx applyLowFreqStartupCorrection_(const Cx& x, long outputCount) const;
    Cx applyLowFreqSteadyPhaseCorrection_(const Cx& x, long outputCount) const;
    Cx envelopeToComplex_(const EnvelopeSignal& x, double fcHz) const;
    Cx addNoise_(const Cx& x, double sigma, uint32_t& seed);
    Cx applyMixerToIF_(const Cx& x, double inputFcHz, double timeNow) const;
    Cx applyChannelDelay_(const Cx& x, ChannelState& st) const;
    Cx applyDDCToBaseband_(const Cx& x, double timeNow) const;
    Cx applyPhaseImbalance_(const Cx& x) const;
    Cx applyADC_(const Cx& x) const;
    Cx applyStage_(const Cx& x, const Cx& gain, SelectedGCType gcType,
                   double toiOut, double dbc1Out, double psat, double gcSat,
                   const GCompTable& table) const;

    // ========== 增益压缩 ==========
    double applyCompressionMagnitude_(double ain, double gainAbs, SelectedGCType gcType,
                                      double toiOut, double dbc1Out, double psat,
                                      double gcSat, const GCompTable& table) const;
    double applyTOI_(double ain, double gainAbs, double toiOut) const;
    double applydBc1_(double ain, double gainAbs, double dbc1Out) const;
    double applyTOIdBc1_(double ain, double gainAbs, double toiOut, double dbc1Out) const;
    double applyPSat_(double ain, double gainAbs, double psat, double gcSat) const;
    double applyRapp_(double ain, double gainAbs, double psat) const;
    double applyTableCompression_(double ain, double gainAbs, const GCompTable& table) const;

    // ========== 随机数 ==========
    double randUniform_(uint32_t& seed) const;
    double randn_(uint32_t& seed) const;

    // ========== BPF ==========
    Cx runBpfFilter_(const Cx& x, ChannelState& st);
    Cx runBiquad_(const Cx& x, BiquadState& s);

    // ========== 静态工具函数 ==========
    static double dbToLinVoltage(double db);
    static double linToDbVoltage(double lin);
    static double wattToDbm(double w);
    static double dbmToWatt(double dbm);
    static double wattToPeakVoltage(double w, double r);
    static double peakVoltageToWatt(double v, double r);
    static double peakVoltageToDbm(double v, double r);
    static double dbmToPeakVoltage(double dbm, double r);
    static double deg2rad(double x);
    static double clamp(double x, double lo, double hi);

    // ========== 参数解析 ==========
    static SelectedGCType ConvertStringToGCType(const std::string& value);
    static Cx ParseComplex(const std::string& str);
    static std::vector<double> ParseDoubleArray(const std::string& str);

    static constexpr double kPi = 3.14159265358979323846;
};

RegAlgo(RADAR_Rx_4x4_Block);

#endif // RADAR_RX_4X4_BLOCK_H
