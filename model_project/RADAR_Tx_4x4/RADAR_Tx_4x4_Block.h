#ifndef RADAR_TX_4X4_BLOCK_H
#define RADAR_TX_4X4_BLOCK_H

#include "RADAR_Tx_4x4.h"
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

class SYSTEMVUEMODELBUILDER_API RADAR_Tx_4x4_Block : public Block
{
public:
    RADAR_Tx_4x4_Block(const std::string& name);
    ~RADAR_Tx_4x4_Block() = default;

    bool Setup() override;
    bool Initialize() override;
    bool Run() override;

    int GetBusChannelCount() const override { return NumTxAnt_; }

    void SetDefaultParameters();
    void SetParameters();

private:
    bool ModelSetup();
    void initFromBusConnections_();
    bool DataStreamRun();
    bool TimeDrivenRun();

    // 仅用于端口注册的算法实例
    std::unique_ptr<RADAR_Tx_4x4> m_algo;

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
    double In_CenterFreq_;
    int BB_UpSamplingRatio_;
    double RC_ExcessBW_;
    double PhaseImbalance_;
    int DAC_NBits_;
    int DAC_UpSamplingRatio_;
    int NumTxAnt_;
    double ChannelDelay_;

    double NoiseFigure_RF_Gain_;
    double NoiseFigure_IF_Gain_;
    double NoiseFigure_Mixer_;

    SelectedGCType GCType_RF_Gain_;
    double TOIout_RF_Gain_;
    double dBc1out_RF_Gain_;
    double PSat_RF_Gain_;
    double GCSat_RF_Gain_;
    int RappS_RF_Gain_;
    std::vector<double> GComp_RF_Gain_Vec_;

    SelectedGCType GCType_IF_Gain_;
    double TOIout_IF_Gain_;
    double dBc1out_IF_Gain_;
    double PSat_IF_Gain_;
    double GCSat_IF_Gain_;
    int RappS_IF_Gain_;
    std::vector<double> GComp_IF_Gain_Vec_;

    // ========== 运行时状态 ==========
    size_t inBusSize_;
    size_t outBusSize_;
    size_t activeChannels_;
    double sampleRateHz_;
    double timeStepSec_;
    double outputSampleRateHz_;
    double outputTimeStepSec_;
    int bbUp_;
    int dacUp_;
    int outRate_;
    int channelDelaySamples_;
    bool noisePrepared_;
    double noiseSigmaRF_;
    double noiseSigmaIF_;
    double noiseSigmaMixer_;
    bool ifBpfEnabled_;
    bool rfBpfEnabled_;
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
        std::deque<Cx> ducFirState;
        Cx ducHold;
        BiquadState ifBpfSec1, ifBpfSec2;
        BiquadState rfBpfSec1, rfBpfSec2;
        std::deque<Cx> delayLine;
        uint32_t seedRF, seedIF, seedMixer;
        unsigned long long outputCount;

        // V8: 用于记录包络变化，抑制镜像/波纹强度
        double lastRfAbs;
        double edgeRippleState;

        // V17: 上升/下降状态，形成"蝉翼"非对称波纹
        double riseEdgeState;
        double fallEdgeState;

        // V16/V17: 脉冲位置，仅用于非常浅的平板凹陷
        bool inPulse;
        unsigned long long pulseSampleIndex;

        ChannelState();
        void resetRuntime();
    };

    std::vector<ChannelState> ch_;
    GCompTable rfTable_;
    GCompTable ifTable_;
    std::vector<double> ducFir_;

    BiquadState ifBpfProtoSec1_, ifBpfProtoSec2_;
    BiquadState rfBpfProtoSec1_, rfBpfProtoSec2_;

    // ========== 时间驱动模式缓冲区 ==========
    struct TDRFrame {
        std::vector<Cx> data;       // 输入: 每通道一个 Cx
        double timeNow;
    };
    std::deque<TDRFrame> m_inputBuffer;
    std::queue<std::vector<EnvelopeSignal>> m_outputQueue;

    // ========== 初始化辅助 ==========
    bool prepareTables_();
    bool prepareNoise_();
    bool parseGCompArray_(const std::vector<double>& data, GCompTable& table) const;
    void buildRaisedCosineFir_();
    void configureIfBpf_();
    void configureRfBpf_();
    void resetChannelStates_();
    int computeChannelDelaySamples_() const;

    // ========== 信号处理 ==========
    Cx runDucInterpolationFir_(const Cx& x, ChannelState& st);
    Cx runBiquad_(const Cx& x, BiquadState& s);
    Cx runIfBpf_(const Cx& x, ChannelState& st);
    Cx runRfBpf_(const Cx& x, ChannelState& st);
    Cx applyChannelDelay_(const Cx& x, ChannelState& st);
    Cx applyInputCenterFrequency_(const Cx& x, double timeNow) const;
    Cx applyDUCToIFEnvelope_(const Cx& x, double timeNow) const;
    Cx applyFcChangeImage_(const Cx& idealEnvelope, double timeNow, ChannelState& st);
    Cx applyFinalComplexPhaseCorrection_(const Cx& x, double timeNow) const;
    Cx applyMixerToRFEnvelope_(const Cx& x, double timeNow) const;
    double applyDAC_(double x) const;
    Cx addNoise_(const Cx& x, double sigma, uint32_t& seed);
    Cx applyStage_(const Cx& x, const Cx& gain, SelectedGCType gcType,
                   double toiOut, double dbc1Out, double psat, double gcSat,
                   int rappS, const GCompTable& table) const;

    // ========== 增益压缩 ==========
    double applyCompressionMagnitude_(double ain, double gainAbs, SelectedGCType gcType,
                                      double toiOut, double dbc1Out, double psat,
                                      double gcSat, int rappS, const GCompTable& table) const;
    Cx applyTableCompressionComplex_(const Cx& yLinear, double ain, double gainAbs,
                                     SelectedGCType gcType, const GCompTable& table) const;
    bool lookupTable_(double pinDbm, const GCompTable& table,
                      double& gainChangeDb, double& phaseChangeDeg) const;
    double applyTOI_(double ain, double gainAbs, double toiOut) const;
    double applydBc1_(double ain, double gainAbs, double dbc1Out) const;
    double applyTOIdBc1_(double ain, double gainAbs, double toiOut, double dbc1Out) const;
    double applyPSat_(double ain, double gainAbs, double psat, double gcSat) const;
    double applyRapp_(double ain, double gainAbs, double psat, int rappS) const;
    double applyTableCompressionMagnitude_(double ain, double gainAbs, const GCompTable& table) const;

    // ========== 随机数 ==========
    double randUniform_(uint32_t& seed) const;
    double randn_(uint32_t& seed) const;

    // ========== 静态工具函数 ==========
    static double raisedCosineImpulse_(double t, double alpha);
    static double sinc_(double x);
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

RegAlgo(RADAR_Tx_4x4_Block);

#endif // RADAR_TX_4X4_BLOCK_H
