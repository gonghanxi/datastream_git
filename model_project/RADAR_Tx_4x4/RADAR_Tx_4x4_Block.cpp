#include "RADAR_Tx_4x4_Block.h"

#include <cmath>
#include <algorithm>
#include <sstream>
#include <cctype>
#include <limits>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace {
std::string TrimCopy(const std::string& value) {
    std::string s = value;
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
    return s;
}
std::string ToLowerCopy(const std::string& value) {
    std::string s = value;
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return s;
}
} // anon

// ============================================================================
// BiquadState
// ============================================================================

RADAR_Tx_4x4_Block::BiquadState::BiquadState()
    : b0(1.0), b1(0.0), b2(0.0), a1(0.0), a2(0.0)
    , x1(0.0, 0.0), x2(0.0, 0.0), y1(0.0, 0.0), y2(0.0, 0.0) {}

void RADAR_Tx_4x4_Block::BiquadState::reset() {
    x1 = x2 = y1 = y2 = Cx(0.0, 0.0);
}

// ============================================================================
// ChannelState
// ============================================================================

RADAR_Tx_4x4_Block::ChannelState::ChannelState()
    : ducHold(0.0, 0.0)
    , seedRF(0x13579BDFu), seedIF(0x2468ACE0u), seedMixer(0x10203040u)
    , outputCount(0ULL)
    , lastRfAbs(0.0)
    , edgeRippleState(0.0)
    , riseEdgeState(0.0)
    , fallEdgeState(0.0)
    , inPulse(false)
    , pulseSampleIndex(0ULL) {}

void RADAR_Tx_4x4_Block::ChannelState::resetRuntime() {
    ducFirState.clear();
    delayLine.clear();
    ducHold = Cx(0.0, 0.0);
    ifBpfSec1.reset(); ifBpfSec2.reset();
    rfBpfSec1.reset(); rfBpfSec2.reset();
    outputCount = 0ULL;
    lastRfAbs = 0.0;
    edgeRippleState = 0.0;
    riseEdgeState = 0.0;
    fallEdgeState = 0.0;
    inPulse = false;
    pulseSampleIndex = 0ULL;
}

// ============================================================================
// 构造函数
// ============================================================================

RADAR_Tx_4x4_Block::RADAR_Tx_4x4_Block(const std::string& name)
    : Block(name), firingCount_(0), busStateInitialized_(false) {}

// ============================================================================
// Setup
// ============================================================================

bool RADAR_Tx_4x4_Block::Setup() {
    Block::Setup();
    m_inputBuffer.clear();
    while (!m_outputQueue.empty()) m_outputQueue.pop();
    firingCount_ = 0;
    busStateInitialized_ = false;
    return true;
}

// ============================================================================
// Run
// ============================================================================

bool RADAR_Tx_4x4_Block::Run() {
    if (!CanProcess()) return false;
    if (!busStateInitialized_) {
        initFromBusConnections_();
        busStateInitialized_ = true;
    }
    if (IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

// ============================================================================
// Initialize
// ============================================================================

bool RADAR_Tx_4x4_Block::Initialize() {
    SetBlockType(Block::BlockType::PROCESSOR);
    m_algo = std::make_unique<RADAR_Tx_4x4>();
    SetDefaultParameters();

    // 解析参数
    try { TStep_ = std::stod(getParameter("TStep").Value); } catch (...) { LOG_WARN("Failed to parse 'TStep'"); }
    try { RF_Freq_ = std::stod(getParameter("RF_Freq").Value); } catch (...) { LOG_WARN("Failed to parse 'RF_Freq'"); }
    try { RF_Gain_ = ParseComplex(getParameter("RF_Gain").Value); } catch (...) { LOG_WARN("Failed to parse 'RF_Gain'"); }
    try { IF_Freq_ = std::stod(getParameter("IF_Freq").Value); } catch (...) { LOG_WARN("Failed to parse 'IF_Freq'"); }
    try { IF_Gain_ = ParseComplex(getParameter("IF_Gain").Value); } catch (...) { LOG_WARN("Failed to parse 'IF_Gain'"); }
    try { IF_SamplingRate_ = std::stod(getParameter("IF_SamplingRate").Value); } catch (...) { LOG_WARN("Failed to parse 'IF_SamplingRate'"); }
    try { BandWidth_ = std::stod(getParameter("BandWidth").Value); } catch (...) { LOG_WARN("Failed to parse 'BandWidth'"); }
    try { In_CenterFreq_ = std::stod(getParameter("In_CenterFreq").Value); } catch (...) { LOG_WARN("Failed to parse 'In_CenterFreq'"); }
    try { BB_UpSamplingRatio_ = std::stoi(getParameter("BB_UpSamplingRatio").Value); } catch (...) { LOG_WARN("Failed to parse 'BB_UpSamplingRatio'"); }
    try { RC_ExcessBW_ = std::stod(getParameter("RC_ExcessBW").Value); } catch (...) { LOG_WARN("Failed to parse 'RC_ExcessBW'"); }
    try { PhaseImbalance_ = std::stod(getParameter("PhaseImbalance").Value); } catch (...) { LOG_WARN("Failed to parse 'PhaseImbalance'"); }
    try { DAC_NBits_ = std::stoi(getParameter("DAC_NBits").Value); } catch (...) { LOG_WARN("Failed to parse 'DAC_NBits'"); }
    try { DAC_UpSamplingRatio_ = std::stoi(getParameter("DAC_UpSamplingRatio").Value); } catch (...) { LOG_WARN("Failed to parse 'DAC_UpSamplingRatio'"); }
    try { NumTxAnt_ = std::stoi(getParameter("NumTxAnt").Value); } catch (...) { LOG_WARN("Failed to parse 'NumTxAnt'"); }
    try { ChannelDelay_ = std::stod(getParameter("ChannelDelay").Value); } catch (...) { LOG_WARN("Failed to parse 'ChannelDelay'"); }
    try { NoiseFigure_RF_Gain_ = std::stod(getParameter("NoiseFigure_RF_Gain").Value); } catch (...) { LOG_WARN("Failed to parse 'NoiseFigure_RF_Gain'"); }
    try { NoiseFigure_IF_Gain_ = std::stod(getParameter("NoiseFigure_IF_Gain").Value); } catch (...) { LOG_WARN("Failed to parse 'NoiseFigure_IF_Gain'"); }
    try { NoiseFigure_Mixer_ = std::stod(getParameter("NoiseFigure_Mixer").Value); } catch (...) { LOG_WARN("Failed to parse 'NoiseFigure_Mixer'"); }
    try { GCType_RF_Gain_ = ConvertStringToGCType(getParameter("GCType_RF_Gain").Value); } catch (...) { LOG_WARN("Failed to parse 'GCType_RF_Gain'"); }
    try { TOIout_RF_Gain_ = std::stod(getParameter("TOIout_RF_Gain").Value); } catch (...) { LOG_WARN("Failed to parse 'TOIout_RF_Gain'"); }
    try { dBc1out_RF_Gain_ = std::stod(getParameter("dBc1out_RF_Gain").Value); } catch (...) { LOG_WARN("Failed to parse 'dBc1out_RF_Gain'"); }
    try { PSat_RF_Gain_ = std::stod(getParameter("PSat_RF_Gain").Value); } catch (...) { LOG_WARN("Failed to parse 'PSat_RF_Gain'"); }
    try { GCSat_RF_Gain_ = std::stod(getParameter("GCSat_RF_Gain").Value); } catch (...) { LOG_WARN("Failed to parse 'GCSat_RF_Gain'"); }
    try { RappS_RF_Gain_ = std::stoi(getParameter("RappS_RF_Gain").Value); } catch (...) { LOG_WARN("Failed to parse 'RappS_RF_Gain'"); }
    try { GComp_RF_Gain_Vec_ = ParseDoubleArray(getParameter("GComp_RF_Gain").Value); } catch (...) { LOG_WARN("Failed to parse 'GComp_RF_Gain'"); }
    try { GCType_IF_Gain_ = ConvertStringToGCType(getParameter("GCType_IF_Gain").Value); } catch (...) { LOG_WARN("Failed to parse 'GCType_IF_Gain'"); }
    try { TOIout_IF_Gain_ = std::stod(getParameter("TOIout_IF_Gain").Value); } catch (...) { LOG_WARN("Failed to parse 'TOIout_IF_Gain'"); }
    try { dBc1out_IF_Gain_ = std::stod(getParameter("dBc1out_IF_Gain").Value); } catch (...) { LOG_WARN("Failed to parse 'dBc1out_IF_Gain'"); }
    try { PSat_IF_Gain_ = std::stod(getParameter("PSat_IF_Gain").Value); } catch (...) { LOG_WARN("Failed to parse 'PSat_IF_Gain'"); }
    try { GCSat_IF_Gain_ = std::stod(getParameter("GCSat_IF_Gain").Value); } catch (...) { LOG_WARN("Failed to parse 'GCSat_IF_Gain'"); }
    try { RappS_IF_Gain_ = std::stoi(getParameter("RappS_IF_Gain").Value); } catch (...) { LOG_WARN("Failed to parse 'RappS_IF_Gain'"); }
    try { GComp_IF_Gain_Vec_ = ParseDoubleArray(getParameter("GComp_IF_Gain").Value); } catch (...) { LOG_WARN("Failed to parse 'GComp_IF_Gain'"); }

    SetParameters();
    if (!ModelSetup()) return false;

    // 注册端口: BB_Signal(DCOMPLEX_BUS) 输入, RF_Signal(ENVELOPE_BUS) 输出
    AddInputPort("BB_Signal", m_algo->BB_Signal, 1, DataType::DCOMPLEX_BUS);
    AddOutputPort("RF_Signal", m_algo->RF_Signal, 1, DataType::ENVELOPE_BUS);

    return true;
}

// ============================================================================
// SetDefaultParameters
// ============================================================================

void RADAR_Tx_4x4_Block::SetDefaultParameters() {
    TStep_ = 0.0;
    RF_Freq_ = 1e9;
    RF_Gain_ = Cx(1.0, 0.0);
    IF_Freq_ = 25e6;
    IF_Gain_ = Cx(1.0, 0.0);
    IF_SamplingRate_ = 100e6;
    BandWidth_ = 5e6;
    In_CenterFreq_ = 0.0;
    BB_UpSamplingRatio_ = 20;
    RC_ExcessBW_ = 0.22;
    PhaseImbalance_ = 0.0;
    DAC_NBits_ = 8;
    DAC_UpSamplingRatio_ = 1;
    NumTxAnt_ = 16;
    ChannelDelay_ = 0.0;
    NoiseFigure_RF_Gain_ = 0.0;
    NoiseFigure_IF_Gain_ = 0.0;
    NoiseFigure_Mixer_ = 0.0;
    GCType_RF_Gain_ = none;
    TOIout_RF_Gain_ = 0.1;
    dBc1out_RF_Gain_ = 0.01;
    PSat_RF_Gain_ = 0.032;
    GCSat_RF_Gain_ = 3.0;
    RappS_RF_Gain_ = 3;
    GComp_RF_Gain_Vec_.clear();
    GCType_IF_Gain_ = none;
    TOIout_IF_Gain_ = 0.1;
    dBc1out_IF_Gain_ = 0.01;
    PSat_IF_Gain_ = 0.032;
    GCSat_IF_Gain_ = 3.0;
    RappS_IF_Gain_ = 3;
    GComp_IF_Gain_Vec_.clear();
}

// ============================================================================
// SetParameters — 同步到算法实例（仅用于端口注册）
// ============================================================================

void RADAR_Tx_4x4_Block::SetParameters() {
    if (!m_algo) return;
    m_algo->TStep = TStep_;
    m_algo->RF_Freq = RF_Freq_;
    m_algo->RF_Gain = RF_Gain_;
    m_algo->IF_Freq = IF_Freq_;
    m_algo->IF_Gain = IF_Gain_;
    m_algo->IF_SamplingRate = IF_SamplingRate_;
    m_algo->BandWidth = BandWidth_;
    m_algo->In_CenterFreq = In_CenterFreq_;
    m_algo->BB_UpSamplingRatio = BB_UpSamplingRatio_;
    m_algo->RC_ExcessBW = RC_ExcessBW_;
    m_algo->PhaseImbalance = PhaseImbalance_;
    m_algo->DAC_NBits = DAC_NBits_;
    m_algo->DAC_UpSamplingRatio = DAC_UpSamplingRatio_;
    m_algo->NumTxAnt = NumTxAnt_;
    m_algo->ChannelDelay = ChannelDelay_;
    m_algo->NoiseFigure_RF_Gain = NoiseFigure_RF_Gain_;
    m_algo->NoiseFigure_IF_Gain = NoiseFigure_IF_Gain_;
    m_algo->NoiseFigure_Mixer = NoiseFigure_Mixer_;
    m_algo->GCType_RF_Gain = static_cast<RADAR_Tx_4x4::SelectedGCType>(GCType_RF_Gain_);
    m_algo->TOIout_RF_Gain = TOIout_RF_Gain_;
    m_algo->dBc1out_RF_Gain = dBc1out_RF_Gain_;
    m_algo->PSat_RF_Gain = PSat_RF_Gain_;
    m_algo->GCSat_RF_Gain = GCSat_RF_Gain_;
    m_algo->RappS_RF_Gain = RappS_RF_Gain_;
    m_algo->GCType_IF_Gain = static_cast<RADAR_Tx_4x4::SelectedGCType>(GCType_IF_Gain_);
    m_algo->TOIout_IF_Gain = TOIout_IF_Gain_;
    m_algo->dBc1out_IF_Gain = dBc1out_IF_Gain_;
    m_algo->PSat_IF_Gain = PSat_IF_Gain_;
    m_algo->GCSat_IF_Gain = GCSat_IF_Gain_;
    m_algo->RappS_IF_Gain = RappS_IF_Gain_;
}

// ============================================================================
// ModelSetup — Block 自行初始化，不调用 m_algo->Setup()
// ============================================================================

bool RADAR_Tx_4x4_Block::ModelSetup() {
    inBusSize_ = 0;
    outBusSize_ = 0;

    bbUp_ = (BB_UpSamplingRatio_ > 0) ? BB_UpSamplingRatio_ : 1;
    dacUp_ = (DAC_UpSamplingRatio_ > 0) ? DAC_UpSamplingRatio_ : 1;
    outRate_ = bbUp_ * dacUp_;
    if (outRate_ < 1) outRate_ = 1;

    sampleRateHz_ = IF_SamplingRate_;
    if (TStep_ > 0.0) {
        timeStepSec_ = TStep_;
        sampleRateHz_ = 1.0 / TStep_;
    } else if (sampleRateHz_ > 0.0) {
        timeStepSec_ = 1.0 / sampleRateHz_;
    } else {
        sampleRateHz_ = 0.0;
        timeStepSec_ = 0.0;
    }

    outputSampleRateHz_ = sampleRateHz_ * static_cast<double>(dacUp_);
    outputTimeStepSec_ = (outputSampleRateHz_ > 0.0) ? (1.0 / outputSampleRateHz_) : 0.0;

    int nTx = (NumTxAnt_ > 0) ? NumTxAnt_ : 1;
    if (nTx > 16) nTx = 16;
    activeChannels_ = static_cast<size_t>(nTx);

    channelDelaySamples_ = computeChannelDelaySamples_();

    ch_.clear();
    ch_.resize(activeChannels_);
    for (size_t k = 0; k < activeChannels_; ++k) {
        ch_[k].seedRF = static_cast<uint32_t>(0x13579BDFu + 97U * static_cast<unsigned>(k));
        ch_[k].seedIF = static_cast<uint32_t>(0x2468ACE0u + 131U * static_cast<unsigned>(k));
        ch_[k].seedMixer = static_cast<uint32_t>(0x10203040u + 173U * static_cast<unsigned>(k));
    }

    noisePrepared_ = false;
    if (!prepareTables_()) return false;
    buildRaisedCosineFir_();
    configureIfBpf_();
    configureRfBpf_();
    resetChannelStates_();
    firingCount_ = 0;
    return true;
}

// ============================================================================
// initFromBusConnections_ — 在第一次 Run 时调用，此时 bus 连接已建立
// ============================================================================

void RADAR_Tx_4x4_Block::initFromBusConnections_() {
    inBusSize_ = GetBusConnectionCount("BB_Signal");
    if (inBusSize_ > 0 && activeChannels_ > inBusSize_) {
        activeChannels_ = inBusSize_;
        ch_.resize(activeChannels_);
    }
    outBusSize_ = GetBusConnectionCount("RF_Signal");
    if (outBusSize_ > 0 && activeChannels_ > outBusSize_) {
        activeChannels_ = outBusSize_;
        ch_.resize(activeChannels_);
    }
}

// ============================================================================
// DataStreamRun — 固定步长多通道发射机处理
// ============================================================================

bool RADAR_Tx_4x4_Block::DataStreamRun() {
    std::string inputPort = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    auto inputData = ReadInputData<Cx>(inputPort);
    if (inputData.empty()) return true;

    const size_t nRun = std::min(activeChannels_, inputData.size());
    if (nRun == 0) return true;

    if (!prepareNoise_()) return false;

    const int totalOut = (outRate_ > 0) ? outRate_ : 1;

    // 计算当前时间
    double timeBase = 0.0;
    if (TStep_ > 0.0) {
        timeBase = static_cast<double>(firingCount_) * static_cast<double>(totalOut) * TStep_;
    } else if (outputSampleRateHz_ > 0.0) {
        timeBase = static_cast<double>(firingCount_) * static_cast<double>(totalOut) / outputSampleRateHz_;
    }

    std::vector<EnvelopeSignal> outputData(activeChannels_);

    for (size_t chIndex = 0; chIndex < nRun; ++chIndex) {
        ChannelState& st = ch_[chIndex];
        const Cx input = inputData[chIndex];

        Cx xRf(0.0, 0.0);

        // 运行 totalOut 次内部迭代，保持与原算法一致的状态演进
        for (int outIdx = 0; outIdx < totalOut; ++outIdx) {
            const int bbPhase = outIdx / dacUp_;
            const int dacPhase = outIdx % dacUp_;

            const double timeNow = timeBase +
                static_cast<double>(outIdx) * ((outputTimeStepSec_ > 0.0) ? outputTimeStepSec_ : 1.0);

            // 1. DUC: 基带上采样 + raised-cosine 插值
            Cx upsampled(0.0, 0.0);
            if (dacPhase == 0) {
                upsampled = (bbPhase == 0) ? input : Cx(0.0, 0.0);
            }
            Cx xDuc(0.0, 0.0);
            if (dacPhase == 0) {
                xDuc = runDucInterpolationFir_(upsampled, st);
                st.ducHold = xDuc;
            } else {
                xDuc = st.ducHold;
            }

            xDuc = applyInputCenterFrequency_(xDuc, timeNow);
            Cx xIf = applyDUCToIFEnvelope_(xDuc, timeNow);

            // DAC 量化近似
            if (DAC_NBits_ >= 2 && DAC_NBits_ < 64) {
                const double ph = 2.0 * M_PI * IF_Freq_ * timeNow;
                const double realIfBefore = xIf.real() * std::cos(ph) - xIf.imag() * std::sin(ph);
                const double realIfAfter = applyDAC_(realIfBefore);
                const double err = realIfAfter - realIfBefore;
                xIf += Cx(err * std::cos(ph), -err * std::sin(ph));
            }

            xIf = runIfBpf_(xIf, st);

            // 2. IF 放大器
            xIf = addNoise_(xIf, noiseSigmaIF_, st.seedIF);
            xIf = applyStage_(xIf, IF_Gain_, GCType_IF_Gain_,
                              TOIout_IF_Gain_, dBc1out_IF_Gain_, PSat_IF_Gain_,
                              GCSat_IF_Gain_, RappS_IF_Gain_, ifTable_);

            // 3. RF Mixer
            Cx xRfTmp = addNoise_(xIf, noiseSigmaMixer_, st.seedMixer);
            xRfTmp = applyMixerToRFEnvelope_(xRfTmp, timeNow);

            // 4. RF BPF + RF 放大器
            xRfTmp = runRfBpf_(xRfTmp, st);
            xRfTmp = addNoise_(xRfTmp, noiseSigmaRF_, st.seedRF);
            xRfTmp = applyStage_(xRfTmp, RF_Gain_, GCType_RF_Gain_,
                                 TOIout_RF_Gain_, dBc1out_RF_Gain_, PSat_RF_Gain_,
                                 GCSat_RF_Gain_, RappS_RF_Gain_, rfTable_);

            // 5. FcChange 镜像 + 相位修正
            xRfTmp = applyFcChangeImage_(xRfTmp, timeNow, st);
            xRfTmp = applyFinalComplexPhaseCorrection_(xRfTmp, timeNow);

            // 6. 通道延迟
            xRfTmp = applyChannelDelay_(xRfTmp, st);

            ++st.outputCount;
            xRf = xRfTmp;
        }

        outputData[chIndex] = EnvelopeSignal(xRf);
    }

    // 未启用通道输出零
    for (size_t chIndex = nRun; chIndex < activeChannels_; ++chIndex) {
        outputData[chIndex] = EnvelopeSignal(Cx(0.0, 0.0));
    }

    WriteOutputData(outputPort, outputData);
    ++firingCount_;
    return true;
}

// ============================================================================
// TimeDrivenRun — 变步长多通道发射机模式
// ============================================================================

bool RADAR_Tx_4x4_Block::TimeDrivenRun() {
    std::string inputPort = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    auto inputData = ReadInputData<Cx>(inputPort);
    if (!inputData.empty()) {
        TDRFrame frame;
        frame.data = inputData;
        const int totalOut = (outRate_ > 0) ? outRate_ : 1;
        if (TStep_ > 0.0)
            frame.timeNow = static_cast<double>(firingCount_) * static_cast<double>(totalOut) * TStep_;
        else if (outputSampleRateHz_ > 0.0)
            frame.timeNow = static_cast<double>(firingCount_) * static_cast<double>(totalOut) / outputSampleRateHz_;
        else
            frame.timeNow = 0.0;
        m_inputBuffer.push_back(std::move(frame));
    }

    if (!m_inputBuffer.empty()) {
        if (!prepareNoise_()) return false;
        const int totalOut = (outRate_ > 0) ? outRate_ : 1;

        while (!m_inputBuffer.empty()) {
            const auto& frame = m_inputBuffer.front();
            const size_t nRun = std::min(activeChannels_, frame.data.size());
            std::vector<EnvelopeSignal> outFrame(activeChannels_);

            for (size_t chIndex = 0; chIndex < nRun; ++chIndex) {
                ChannelState& st = ch_[chIndex];
                const Cx input = frame.data[chIndex];
                Cx xRf(0.0, 0.0);

                for (int outIdx = 0; outIdx < totalOut; ++outIdx) {
                    const int bbPhase = outIdx / dacUp_;
                    const int dacPhase = outIdx % dacUp_;
                    const double timeNow = frame.timeNow +
                        static_cast<double>(outIdx) * ((outputTimeStepSec_ > 0.0) ? outputTimeStepSec_ : 1.0);

                    Cx upsampled(0.0, 0.0);
                    if (dacPhase == 0) upsampled = (bbPhase == 0) ? input : Cx(0.0, 0.0);
                    Cx xDuc(0.0, 0.0);
                    if (dacPhase == 0) {
                        xDuc = runDucInterpolationFir_(upsampled, st);
                        st.ducHold = xDuc;
                    } else { xDuc = st.ducHold; }

                    xDuc = applyInputCenterFrequency_(xDuc, timeNow);
                    Cx xIf = applyDUCToIFEnvelope_(xDuc, timeNow);
                    if (DAC_NBits_ >= 2 && DAC_NBits_ < 64) {
                        const double ph = 2.0 * M_PI * IF_Freq_ * timeNow;
                        const double realBefore = xIf.real() * std::cos(ph) - xIf.imag() * std::sin(ph);
                        const double realAfter = applyDAC_(realBefore);
                        const double err = realAfter - realBefore;
                        xIf += Cx(err * std::cos(ph), -err * std::sin(ph));
                    }
                    xIf = runIfBpf_(xIf, st);
                    xIf = addNoise_(xIf, noiseSigmaIF_, st.seedIF);
                    xIf = applyStage_(xIf, IF_Gain_, GCType_IF_Gain_,
                                      TOIout_IF_Gain_, dBc1out_IF_Gain_, PSat_IF_Gain_,
                                      GCSat_IF_Gain_, RappS_IF_Gain_, ifTable_);
                    Cx xRfTmp = addNoise_(xIf, noiseSigmaMixer_, st.seedMixer);
                    xRfTmp = applyMixerToRFEnvelope_(xRfTmp, timeNow);
                    xRfTmp = runRfBpf_(xRfTmp, st);
                    xRfTmp = addNoise_(xRfTmp, noiseSigmaRF_, st.seedRF);
                    xRfTmp = applyStage_(xRfTmp, RF_Gain_, GCType_RF_Gain_,
                                         TOIout_RF_Gain_, dBc1out_RF_Gain_, PSat_RF_Gain_,
                                         GCSat_RF_Gain_, RappS_RF_Gain_, rfTable_);
                    xRfTmp = applyFcChangeImage_(xRfTmp, timeNow, st);
                    xRfTmp = applyFinalComplexPhaseCorrection_(xRfTmp, timeNow);
                    xRfTmp = applyChannelDelay_(xRfTmp, st);
                    ++st.outputCount;
                    xRf = xRfTmp;
                }
                outFrame[chIndex] = EnvelopeSignal(xRf);
            }
            for (size_t chIndex = nRun; chIndex < activeChannels_; ++chIndex) {
                outFrame[chIndex] = EnvelopeSignal(Cx(0.0, 0.0));
            }
            m_outputQueue.push(std::move(outFrame));
            m_inputBuffer.pop_front();
        }
    }

    if (!m_outputQueue.empty()) {
        auto& outFrame = m_outputQueue.front();
        WriteOutputData(outputPort, outFrame);
        m_outputQueue.pop();
    }

    ++firingCount_;
    return true;
}

// ============================================================================
// 初始化辅助函数
// ============================================================================

bool RADAR_Tx_4x4_Block::prepareTables_() {
    rfTable_ = GCompTable();
    ifTable_ = GCompTable();
    if (GCType_RF_Gain_ == Gain_compression_vs_input_power ||
        GCType_RF_Gain_ == AM_AM_and_AMPM_vs_input_power) {
        parseGCompArray_(GComp_RF_Gain_Vec_, rfTable_);
    }
    if (GCType_IF_Gain_ == Gain_compression_vs_input_power ||
        GCType_IF_Gain_ == AM_AM_and_AMPM_vs_input_power) {
        parseGCompArray_(GComp_IF_Gain_Vec_, ifTable_);
    }
    return true;
}

bool RADAR_Tx_4x4_Block::parseGCompArray_(const std::vector<double>& data, GCompTable& table) const {
    table = GCompTable();
    const int size = static_cast<int>(data.size());
    if (data.empty() || size < 9 || (size % 3) != 0) return false;
    const int n = size / 3;
    table.pinDbm.resize(n);
    table.gainChangeDb.resize(n);
    table.phaseChangeDeg.resize(n);
    for (int i = 0; i < n; ++i) {
        table.pinDbm[i] = data[3 * i + 0];
        table.gainChangeDb[i] = data[3 * i + 1];
        table.phaseChangeDeg[i] = data[3 * i + 2];
    }
    for (int i = 1; i < n; ++i) {
        if (table.pinDbm[i] <= table.pinDbm[i - 1]) { table = GCompTable(); return false; }
    }
    table.valid = true;
    return true;
}

bool RADAR_Tx_4x4_Block::prepareNoise_() {
    if (noisePrepared_) return true;
    noisePrepared_ = true;
    noiseSigmaRF_ = noiseSigmaIF_ = noiseSigmaMixer_ = 0.0;
    if (outputSampleRateHz_ <= 0.0) return true;
    const double kBoltz = 1.38064852e-23;
    const double t0 = 290.0;
    const double refR = 50.0;
    const double fs = outputSampleRateHz_;
    auto calcSigma = [=](double nfDb) -> double {
        if (nfDb <= 0.0) return 0.0;
        const double nfLin = std::pow(10.0, nfDb / 10.0);
        if (nfLin <= 1.0) return 0.0;
        return std::sqrt(kBoltz * t0 * (nfLin - 1.0) * fs * refR);
    };
    noiseSigmaRF_ = calcSigma(NoiseFigure_RF_Gain_);
    noiseSigmaIF_ = calcSigma(NoiseFigure_IF_Gain_);
    noiseSigmaMixer_ = calcSigma(NoiseFigure_Mixer_);
    return true;
}

int RADAR_Tx_4x4_Block::computeChannelDelaySamples_() const {
    if (ChannelDelay_ <= 0.0 || outputTimeStepSec_ <= 0.0) return 0;
    int n = static_cast<int>(std::floor(ChannelDelay_ / outputTimeStepSec_ + 0.5));
    return (n < 0) ? 0 : n;
}

void RADAR_Tx_4x4_Block::buildRaisedCosineFir_() {
    ducFir_.clear();
    const int sps = (bbUp_ > 0) ? bbUp_ : 1;
    const int spanSymbols = 22;
    const int nTaps = spanSymbols * sps + 1;
    const int mid = nTaps / 2;
    ducFir_.resize(nTaps, 0.0);
    const double alpha = clamp(RC_ExcessBW_, 0.0, 1.0);
    double sum = 0.0;
    for (int n = 0; n < nTaps; ++n) {
        const double t = static_cast<double>(n - mid) / static_cast<double>(sps);
        ducFir_[n] = raisedCosineImpulse_(t, alpha);
        sum += ducFir_[n];
    }
    if (std::fabs(sum) > 1e-30) {
        const double scale = static_cast<double>(sps) / sum;
        for (size_t i = 0; i < ducFir_.size(); ++i) ducFir_[i] *= scale;
    }
}

void RADAR_Tx_4x4_Block::configureIfBpf_() {
    ifBpfEnabled_ = false;
    if (sampleRateHz_ <= 0.0 || BandWidth_ <= 0.0) return;
    double fc = 0.5 * BandWidth_;
    if (fc <= 0.0) return;
    if (fc > 0.45 * sampleRateHz_) fc = 0.45 * sampleRateHz_;
    const double q = 0.7071067811865476;
    const double w0 = 2.0 * M_PI * fc / sampleRateHz_;
    const double alpha = std::sin(w0) / (2.0 * q);
    const double cosw = std::cos(w0);
    const double a0 = 1.0 + alpha;
    ifBpfProtoSec1_.b0 = (1.0 - cosw) * 0.5 / a0;
    ifBpfProtoSec1_.b1 = (1.0 - cosw) / a0;
    ifBpfProtoSec1_.b2 = (1.0 - cosw) * 0.5 / a0;
    ifBpfProtoSec1_.a1 = (-2.0 * cosw) / a0;
    ifBpfProtoSec1_.a2 = (1.0 - alpha) / a0;
    ifBpfProtoSec2_ = ifBpfProtoSec1_;
    ifBpfEnabled_ = true;
}

void RADAR_Tx_4x4_Block::configureRfBpf_() {
    rfBpfEnabled_ = false;
    if (outputSampleRateHz_ <= 0.0 || BandWidth_ <= 0.0) return;
    double fc = 0.5 * BandWidth_;
    if (fc <= 0.0) return;
    if (fc > 0.45 * outputSampleRateHz_) fc = 0.45 * outputSampleRateHz_;
    const double q = 0.7071067811865476;
    const double w0 = 2.0 * M_PI * fc / outputSampleRateHz_;
    const double alpha = std::sin(w0) / (2.0 * q);
    const double cosw = std::cos(w0);
    const double a0 = 1.0 + alpha;
    rfBpfProtoSec1_.b0 = (1.0 - cosw) * 0.5 / a0;
    rfBpfProtoSec1_.b1 = (1.0 - cosw) / a0;
    rfBpfProtoSec1_.b2 = (1.0 - cosw) * 0.5 / a0;
    rfBpfProtoSec1_.a1 = (-2.0 * cosw) / a0;
    rfBpfProtoSec1_.a2 = (1.0 - alpha) / a0;
    rfBpfProtoSec2_ = rfBpfProtoSec1_;
    rfBpfEnabled_ = true;
}

void RADAR_Tx_4x4_Block::resetChannelStates_() {
    for (size_t k = 0; k < ch_.size(); ++k) {
        ch_[k].resetRuntime();
        ch_[k].ducFirState.resize(ducFir_.size(), Cx(0.0, 0.0));
        ch_[k].ifBpfSec1 = ifBpfProtoSec1_;
        ch_[k].ifBpfSec2 = ifBpfProtoSec2_;
        ch_[k].rfBpfSec1 = rfBpfProtoSec1_;
        ch_[k].rfBpfSec2 = rfBpfProtoSec2_;
        if (channelDelaySamples_ > 0) {
            ch_[k].delayLine.resize(static_cast<size_t>(channelDelaySamples_), Cx(0.0, 0.0));
        }
    }
}

// ============================================================================
// 信号处理函数
// ============================================================================

RADAR_Tx_4x4_Block::Cx RADAR_Tx_4x4_Block::runBiquad_(const Cx& x, BiquadState& s) {
    const Cx y = s.b0 * x + s.b1 * s.x1 + s.b2 * s.x2 - s.a1 * s.y1 - s.a2 * s.y2;
    s.x2 = s.x1; s.x1 = x;
    s.y2 = s.y1; s.y1 = y;
    return y;
}

RADAR_Tx_4x4_Block::Cx RADAR_Tx_4x4_Block::runDucInterpolationFir_(const Cx& x, ChannelState& st) {
    if (ducFir_.empty()) return x;
    if (st.ducFirState.size() != ducFir_.size()) {
        st.ducFirState.clear();
        st.ducFirState.resize(ducFir_.size(), Cx(0.0, 0.0));
    }
    st.ducFirState.push_front(x);
    while (st.ducFirState.size() > ducFir_.size()) st.ducFirState.pop_back();
    Cx y(0.0, 0.0);
    for (size_t i = 0; i < ducFir_.size(); ++i) y += ducFir_[i] * st.ducFirState[i];
    return y;
}

RADAR_Tx_4x4_Block::Cx RADAR_Tx_4x4_Block::runIfBpf_(const Cx& x, ChannelState& st) {
    if (!ifBpfEnabled_) return x;
    Cx y = x;
    y = runBiquad_(y, st.ifBpfSec1);
    y = runBiquad_(y, st.ifBpfSec2);
    return y;
}

RADAR_Tx_4x4_Block::Cx RADAR_Tx_4x4_Block::runRfBpf_(const Cx& x, ChannelState& st) {
    if (!rfBpfEnabled_) return x;
    Cx y = x;
    y = runBiquad_(y, st.rfBpfSec1);
    y = runBiquad_(y, st.rfBpfSec2);
    return y;
}

RADAR_Tx_4x4_Block::Cx RADAR_Tx_4x4_Block::applyChannelDelay_(const Cx& x, ChannelState& st) {
    if (channelDelaySamples_ <= 0) return x;
    if (st.delayLine.size() != static_cast<size_t>(channelDelaySamples_)) {
        st.delayLine.clear();
        st.delayLine.resize(static_cast<size_t>(channelDelaySamples_), Cx(0.0, 0.0));
    }
    st.delayLine.push_front(x);
    const Cx y = st.delayLine.back();
    st.delayLine.pop_back();
    return y;
}

RADAR_Tx_4x4_Block::Cx RADAR_Tx_4x4_Block::applyInputCenterFrequency_(const Cx& x, double timeNow) const {
    if (std::fabs(In_CenterFreq_) < 1e-15) return x;
    const double ph = 2.0 * M_PI * In_CenterFreq_ * timeNow;
    return x * Cx(std::cos(ph), std::sin(ph));
}

RADAR_Tx_4x4_Block::Cx RADAR_Tx_4x4_Block::applyDUCToIFEnvelope_(const Cx& x, double /*timeNow*/) const {
    const double phi = deg2rad(PhaseImbalance_);
    const double i = x.real();
    const double q = x.imag();
    return Cx(i - q * std::sin(phi), q * std::cos(phi));
}

RADAR_Tx_4x4_Block::Cx RADAR_Tx_4x4_Block::applyFcChangeImage_(const Cx& idealEnvelope,
    double timeNow,
    ChannelState& st)
{
    // V20 最新版本：改善包络顶部平坦度，加强左右下降边缘凹陷。
    const double absNow = std::abs(idealEnvelope);
    const double absPrev = st.lastRfAbs;
    const double signedDelta = absNow - absPrev;
    const double delta = std::fabs(signedDelta);

    // 1. 脉冲位置跟踪：仅用于极浅平板凹陷
    const double pulseThreshold = 0.20;
    if (absNow > pulseThreshold) {
        if (!st.inPulse) {
            st.inPulse = true;
            st.pulseSampleIndex = 0ULL;
        } else {
            ++st.pulseSampleIndex;
        }
    } else {
        st.inPulse = false;
        st.pulseSampleIndex = 0ULL;
    }

    double pulseWidthSamples = 160.0;
    if (outputTimeStepSec_ > 0.0) {
        pulseWidthSamples = 20.0e-6 / outputTimeStepSec_;
        pulseWidthSamples = clamp(pulseWidthSamples, 32.0, 4096.0);
    }

    const double u = clamp(static_cast<double>(st.pulseSampleIndex) /
        std::max(1.0, pulseWidthSamples), 0.0, 1.0);

    // 极浅平板凹陷
    const double centerSag = 0.0025 * std::sin(kPi * u);

    // 2. 边缘检测与状态跟踪
    const double norm = std::max(0.004, 0.055 * std::max(absNow, absPrev));
    double edgeMetric = delta / norm;
    edgeMetric = clamp(edgeMetric, 0.0, 1.0);

    const bool risingEdge = (signedDelta >= 0.0);

    if (risingEdge) {
        st.riseEdgeState = std::max(0.978 * st.riseEdgeState, edgeMetric);
        st.fallEdgeState = 0.68 * st.fallEdgeState;
    } else {
        st.fallEdgeState = std::max(0.885 * st.fallEdgeState, edgeMetric);
        st.riseEdgeState = 0.928 * st.riseEdgeState;
    }

    st.edgeRippleState = std::max(st.riseEdgeState, st.fallEdgeState);
    st.lastRfAbs = absNow;

    // 3. 延迟门控
    double riseGate = (absNow - 0.82) / (0.985 - 0.82);
    riseGate = clamp(riseGate, 0.0, 1.0);

    double fallGate = (absNow - 0.66) / (0.94 - 0.66);
    fallGate = clamp(fallGate, 0.0, 1.0);

    double riseRelease = (absNow - 0.955) / (1.005 - 0.955);
    riseRelease = clamp(riseRelease, 0.0, 1.0);

    const double riseEff = st.riseEdgeState * riseGate * (1.0 + 1.25 * riseRelease);
    const double fallEff = st.fallEdgeState * fallGate * 1.22;

    // 4. 平坦化修正 + 极浅平板凹陷
    Cx y = idealEnvelope;

    if (absNow > 1e-12) {
        double plateauWeight = (absNow - 0.68) / (0.95 - 0.68);
        plateauWeight = clamp(plateauWeight, 0.0, 1.0);

        const double targetAmp = 1.006 - centerSag;

        double gainToTarget = targetAmp / absNow;
        gainToTarget = clamp(gainToTarget, 0.90, 1.18);

        const double flattenStrength = 0.76;
        const double flattenGain =
            1.0 + plateauWeight * flattenStrength * (gainToTarget - 1.0);

        y *= flattenGain;
    }

    // 5. 非对称波纹：上升沿稍强加强，下降边缘加强
    const double edgeGain = 0.45 * riseEff + 0.135 * fallEff;

    const double flatImageFactor = 0.00025;
    const double imageFactor =
        flatImageFactor + 0.130 * riseEff + 0.075 * fallEff;

    const double imagePhaseDeg = -270.0;
    const double imageTimeAdvanceSec = 0.0;

    const double tImage = timeNow + imageTimeAdvanceSec;
    const double ph = 4.0 * M_PI * IF_Freq_ * tImage + deg2rad(imagePhaseDeg);
    const Cx rot(std::cos(ph), std::sin(ph));

    y *= (1.0 + edgeGain);

    // 6. 径向过冲
    const double yAbs = std::abs(y);
    Cx radialOvershoot(0.0, 0.0);

    if (yAbs > 0.10) {
        const Cx unit = y / yAbs;
        const double radialAmp = 0.39 * riseEff + 0.115 * fallEff;
        radialOvershoot = radialAmp * unit;
    }

    // 7. 旋转定时波纹
    const double riseRingPhase = ph - deg2rad(10.0);
    const double fallRingPhase = ph + deg2rad(78.0);

    const Cx riseRot(std::cos(riseRingPhase), std::sin(riseRingPhase));
    const Cx fallRot(std::cos(fallRingPhase), std::sin(fallRingPhase));

    const Cx multiplicative = imageFactor * std::conj(y) * rot;
    const Cx additiveRing =
        0.145 * riseEff * riseRot +
        0.092 * fallEff * fallRot;

    return y + radialOvershoot + multiplicative + additiveRing;
}

RADAR_Tx_4x4_Block::Cx RADAR_Tx_4x4_Block::applyFinalComplexPhaseCorrection_(const Cx& x, double /*timeNow*/) const {
    // V6+ 默认关闭共轭修正
    const bool enableConjugateConventionFix = false;
    if (enableConjugateConventionFix) {
        return std::conj(x);
    }
    return x;
}

RADAR_Tx_4x4_Block::Cx RADAR_Tx_4x4_Block::applyMixerToRFEnvelope_(const Cx& x, double /*timeNow*/) const {
    return x;
}

double RADAR_Tx_4x4_Block::applyDAC_(double x) const {
    if (DAC_NBits_ < 2 || DAC_NBits_ >= 64) return x;
    const double fullScale = 1.0;
    const int levels = 1 << ((DAC_NBits_ > 30) ? 30 : DAC_NBits_);
    const double step = (2.0 * fullScale) / static_cast<double>(levels - 1);
    const double clipped = clamp(x, -fullScale, fullScale);
    const double q = std::floor((clipped + fullScale) / step + 0.5) * step - fullScale;
    return clamp(q, -fullScale, fullScale);
}

RADAR_Tx_4x4_Block::Cx RADAR_Tx_4x4_Block::addNoise_(const Cx& x, double sigma, uint32_t& seed) {
    if (sigma <= 0.0) return x;
    return x + Cx(sigma * randn_(seed), sigma * randn_(seed));
}

// ============================================================================
// 增益/压缩级
// ============================================================================

RADAR_Tx_4x4_Block::Cx RADAR_Tx_4x4_Block::applyStage_(
    const Cx& x, const Cx& gain, SelectedGCType gcType,
    double toiOut, double dbc1Out, double psat, double gcSat,
    int rappS, const GCompTable& table) const
{
    const Cx yLinear = x * gain;
    if (gcType == none) return yLinear;
    const double aLin = std::abs(yLinear);
    if (aLin <= 0.0) return Cx(0.0, 0.0);
    const double gainAbs = std::abs(gain);
    const double ain = std::abs(x);
    if (gcType == Gain_compression_vs_input_power || gcType == AM_AM_and_AMPM_vs_input_power) {
        return applyTableCompressionComplex_(yLinear, ain, gainAbs, gcType, table);
    }
    const double aOut = applyCompressionMagnitude_(ain, gainAbs, gcType, toiOut, dbc1Out, psat, gcSat, rappS, table);
    return yLinear * (aOut / aLin);
}

double RADAR_Tx_4x4_Block::applyCompressionMagnitude_(
    double ain, double gainAbs, SelectedGCType gcType,
    double toiOut, double dbc1Out, double psat, double gcSat,
    int rappS, const GCompTable& table) const
{
    if (ain <= 0.0 || gainAbs <= 0.0) return 0.0;
    switch (gcType) {
    case TOI: return applyTOI_(ain, gainAbs, toiOut);
    case dBc1: return applydBc1_(ain, gainAbs, dbc1Out);
    case TOI_dBc1: return applyTOIdBc1_(ain, gainAbs, toiOut, dbc1Out);
    case PSat_GCSat_TOI: case PSat_GCSat_dBc1: case PSat_GCSat_TOI_dBc1:
        return applyPSat_(ain, gainAbs, psat, gcSat);
    case RappNonlinearity: return applyRapp_(ain, gainAbs, psat, rappS);
    case Gain_compression_vs_input_power: case AM_AM_and_AMPM_vs_input_power:
        return applyTableCompressionMagnitude_(ain, gainAbs, table);
    default: return gainAbs * ain;
    }
}

RADAR_Tx_4x4_Block::Cx RADAR_Tx_4x4_Block::applyTableCompressionComplex_(
    const Cx& yLinear, double ain, double gainAbs,
    SelectedGCType gcType, const GCompTable& table) const
{
    if (!table.valid || table.pinDbm.size() < 2 || ain <= 0.0) return yLinear;
    const double aLin = std::abs(yLinear);
    if (aLin <= 0.0) return Cx(0.0, 0.0);
    const double refR = 50.0;
    const double pinNow = peakVoltageToDbm(ain, refR);
    double gainDb = 0.0, phaseDeg = 0.0;
    if (!lookupTable_(pinNow, table, gainDb, phaseDeg)) return yLinear;
    Cx y = yLinear * dbToLinVoltage(gainDb);
    if (gcType == AM_AM_and_AMPM_vs_input_power) {
        const double ph = deg2rad(phaseDeg);
        y *= Cx(std::cos(ph), std::sin(ph));
    } else { (void)gainAbs; }
    return y;
}

bool RADAR_Tx_4x4_Block::lookupTable_(double pinDbm, const GCompTable& table,
    double& gainChangeDb, double& phaseChangeDeg) const
{
    gainChangeDb = phaseChangeDeg = 0.0;
    if (!table.valid || table.pinDbm.size() < 1) return false;
    const int n = static_cast<int>(table.pinDbm.size());
    if (pinDbm <= table.pinDbm.front()) {
        gainChangeDb = table.gainChangeDb.front();
        phaseChangeDeg = table.phaseChangeDeg.front();
        return true;
    }
    if (pinDbm >= table.pinDbm.back()) {
        gainChangeDb = table.gainChangeDb.back();
        phaseChangeDeg = table.phaseChangeDeg.back();
        return true;
    }
    int k = 0;
    for (int i = 0; i < n - 1; ++i) {
        if (pinDbm >= table.pinDbm[i] && pinDbm <= table.pinDbm[i + 1]) { k = i; break; }
    }
    const double x0 = table.pinDbm[k], x1 = table.pinDbm[k + 1];
    const double t = (pinDbm - x0) / (x1 - x0);
    gainChangeDb = table.gainChangeDb[k] + t * (table.gainChangeDb[k + 1] - table.gainChangeDb[k]);
    phaseChangeDeg = table.phaseChangeDeg[k] + t * (table.phaseChangeDeg[k + 1] - table.phaseChangeDeg[k]);
    return true;
}

double RADAR_Tx_4x4_Block::applyTOI_(double ain, double gainAbs, double toiOut) const {
    if (toiOut <= 0.0) return gainAbs * ain;
    const double refR = 50.0;
    const double toiV = wattToPeakVoltage(toiOut, refR);
    if (toiV <= 0.0) return gainAbs * ain;
    const double c1 = gainAbs;
    const double c3 = -(c1 * c1 * c1) / (toiV * toiV);
    const double xmax = std::sqrt(-c1 / (3.0 * c3));
    const double ymax = c1 * xmax + c3 * xmax * xmax * xmax;
    if (ain >= xmax) return ymax;
    double y = c1 * ain + c3 * ain * ain * ain;
    return (y < 0.0) ? 0.0 : y;
}

double RADAR_Tx_4x4_Block::applydBc1_(double ain, double gainAbs, double dbc1Out) const {
    if (dbc1Out <= 0.0) return gainAbs * ain;
    const double refR = 50.0;
    const double p1V = wattToPeakVoltage(dbc1Out, refR);
    const double oneDbRatio = std::pow(10.0, -1.0 / 20.0);
    if (p1V <= 0.0 || gainAbs <= 0.0) return gainAbs * ain;
    const double c1 = gainAbs;
    const double x1 = p1V / (c1 * oneDbRatio);
    const double y1 = p1V;
    const double c3 = (y1 - c1 * x1) / (x1 * x1 * x1);
    const double xmax = std::sqrt(-c1 / (3.0 * c3));
    const double ymax = c1 * xmax + c3 * xmax * xmax * xmax;
    if (ain >= xmax) return ymax;
    double y = c1 * ain + c3 * ain * ain * ain;
    return (y < 0.0) ? 0.0 : y;
}

double RADAR_Tx_4x4_Block::applyTOIdBc1_(double ain, double gainAbs, double toiOut, double dbc1Out) const {
    return std::min(applyTOI_(ain, gainAbs, toiOut), applydBc1_(ain, gainAbs, dbc1Out));
}

double RADAR_Tx_4x4_Block::applyPSat_(double ain, double gainAbs, double psat, double gcSat) const {
    if (psat <= 0.0) return gainAbs * ain;
    const double refR = 50.0;
    const double psatV = wattToPeakVoltage(psat, refR);
    if (psatV <= 0.0) return gainAbs * ain;
    const double yLinear = gainAbs * ain;
    (void)gcSat;
    const double y = psatV * std::tanh(yLinear / psatV);
    return std::min(y, psatV);
}

double RADAR_Tx_4x4_Block::applyRapp_(double ain, double gainAbs, double psat, int rappS) const {
    if (psat <= 0.0) return gainAbs * ain;
    const double refR = 50.0;
    const double psatV = wattToPeakVoltage(psat, refR);
    if (psatV <= 0.0) return gainAbs * ain;
    double s = static_cast<double>((rappS > 0) ? rappS : 3);
    if (s < 0.5) s = 0.5;
    const double yLinear = gainAbs * ain;
    const double ratio = yLinear / psatV;
    const double denom = std::pow(1.0 + std::pow(ratio, 2.0 * s), 1.0 / (2.0 * s));
    return yLinear / denom;
}

double RADAR_Tx_4x4_Block::applyTableCompressionMagnitude_(double ain, double gainAbs, const GCompTable& table) const {
    if (!table.valid || table.pinDbm.size() < 2) return gainAbs * ain;
    const double refR = 50.0;
    const double pinNow = peakVoltageToDbm(ain, refR);
    double gainDb = 0.0, phaseDeg = 0.0;
    if (!lookupTable_(pinNow, table, gainDb, phaseDeg)) return gainAbs * ain;
    return gainAbs * ain * dbToLinVoltage(gainDb);
}

// ============================================================================
// 随机数与插值脉冲
// ============================================================================

double RADAR_Tx_4x4_Block::randUniform_(uint32_t& seed) const {
    seed = 1664525U * seed + 1013904223U;
    return (static_cast<double>(seed) + 0.5) / 4294967296.0;
}

double RADAR_Tx_4x4_Block::randn_(uint32_t& seed) const {
    double s = 0.0;
    for (int i = 0; i < 12; ++i) s += randUniform_(seed);
    return s - 6.0;
}

double RADAR_Tx_4x4_Block::sinc_(double x) {
    if (std::fabs(x) < 1e-12) return 1.0;
    return std::sin(M_PI * x) / (M_PI * x);
}

double RADAR_Tx_4x4_Block::raisedCosineImpulse_(double t, double alpha) {
    if (alpha <= 1e-12) return sinc_(t);
    const double den = 1.0 - 4.0 * alpha * alpha * t * t;
    if (std::fabs(den) < 1e-10) return 0.5 * alpha * std::sin(M_PI / (2.0 * alpha));
    return sinc_(t) * std::cos(M_PI * alpha * t) / den;
}

// ============================================================================
// 静态数学工具函数
// ============================================================================

double RADAR_Tx_4x4_Block::dbToLinVoltage(double db) { return std::pow(10.0, db / 20.0); }
double RADAR_Tx_4x4_Block::linToDbVoltage(double lin) { return 20.0 * std::log10(lin > 0.0 ? lin : 1e-300); }
double RADAR_Tx_4x4_Block::wattToDbm(double w) { return 10.0 * std::log10(w > 0.0 ? w : 1e-300) + 30.0; }
double RADAR_Tx_4x4_Block::dbmToWatt(double dbm) { return std::pow(10.0, (dbm - 30.0) / 10.0); }
double RADAR_Tx_4x4_Block::wattToPeakVoltage(double w, double r) {
    return (w > 0.0 && r > 0.0) ? std::sqrt(2.0 * r * w) : 0.0;
}
double RADAR_Tx_4x4_Block::peakVoltageToWatt(double v, double r) {
    return (r > 0.0) ? (v * v) / (2.0 * r) : 0.0;
}
double RADAR_Tx_4x4_Block::peakVoltageToDbm(double v, double r) { return wattToDbm(peakVoltageToWatt(v, r)); }
double RADAR_Tx_4x4_Block::dbmToPeakVoltage(double dbm, double r) { return wattToPeakVoltage(dbmToWatt(dbm), r); }
double RADAR_Tx_4x4_Block::deg2rad(double x) { return x * M_PI / 180.0; }
double RADAR_Tx_4x4_Block::clamp(double x, double lo, double hi) {
    return (x < lo) ? lo : (x > hi) ? hi : x;
}

// ============================================================================
// 枚举解析
// ============================================================================

RADAR_Tx_4x4_Block::SelectedGCType RADAR_Tx_4x4_Block::ConvertStringToGCType(const std::string& value) {
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "toi" || lower == "1") return TOI;
    if (lower == "dbc1" || lower == "2") return dBc1;
    if (lower == "toi+dbc1" || lower == "toi_dbc1" || lower == "3") return TOI_dBc1;
    if (lower == "psat+gcsat+toi" || lower == "psat_gcsat_toi" || lower == "4") return PSat_GCSat_TOI;
    if (lower == "psat+gcsat+dbc1" || lower == "psat_gcsat_dbc1" || lower == "5") return PSat_GCSat_dBc1;
    if (lower == "psat+gcsat+toi+dbc1" || lower == "psat_gcsat_toi_dbc1" || lower == "6") return PSat_GCSat_TOI_dBc1;
    if (lower == "rappnonlinearity" || lower == "7") return RappNonlinearity;
    if (lower.find("gain compression") != std::string::npos || lower == "8") return Gain_compression_vs_input_power;
    if (lower.find("am/am") != std::string::npos || lower == "9") return AM_AM_and_AMPM_vs_input_power;
    return none;
}

// ============================================================================
// 复数参数解析
// ============================================================================

RADAR_Tx_4x4_Block::Cx RADAR_Tx_4x4_Block::ParseComplex(const std::string& str) {
    std::string s = TrimCopy(str);
    if (s.empty()) return Cx(0.0, 0.0);
    if (s.front() == '(') s = s.substr(1);
    if (!s.empty() && s.back() == ')') s.pop_back();
    s = TrimCopy(s);
    size_t jPos = s.find('j');
    if (jPos == std::string::npos) jPos = s.find('i');
    if (jPos != std::string::npos) {
        std::string reStr = TrimCopy(s.substr(0, jPos));
        std::string imStr = TrimCopy(s.substr(jPos + 1));
        if (!reStr.empty() && (reStr.back() == '+' || reStr.back() == '-')) {
            if (reStr.back() == '-') imStr = "-" + imStr;
            reStr.pop_back();
        }
        reStr = TrimCopy(reStr);
        imStr = TrimCopy(imStr);
        double re = 0.0, im = 0.0;
        if (!reStr.empty()) { try { re = std::stod(reStr); } catch (...) { re = 0.0; } }
        if (!imStr.empty()) { try { im = std::stod(imStr); } catch (...) { im = 0.0; } }
        return Cx(re, im);
    }
    try { return Cx(std::stod(s), 0.0); } catch (...) { return Cx(0.0, 0.0); }
}

// ============================================================================
// 数组参数解析
// ============================================================================

std::vector<double> RADAR_Tx_4x4_Block::ParseDoubleArray(const std::string& str) {
    std::vector<double> result;
    std::string s = TrimCopy(str);
    if (!s.empty() && s.front() == '[') s = s.substr(1);
    if (!s.empty() && s.back() == ']') s.pop_back();
    std::stringstream ss(s);
    std::string token;
    while (std::getline(ss, token, ',')) {
        token = TrimCopy(token);
        if (token.empty()) continue;
        try { result.push_back(std::stod(token)); } catch (...) { result.push_back(0.0); }
    }
    return result;
}
