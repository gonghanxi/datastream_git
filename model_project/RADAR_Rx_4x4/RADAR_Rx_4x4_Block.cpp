#include "RADAR_Rx_4x4_Block.h"

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

RADAR_Rx_4x4_Block::BiquadState::BiquadState()
    : b0(1.0), b1(0.0), b2(0.0), a1(0.0), a2(0.0)
    , x1(0.0, 0.0), x2(0.0, 0.0), y1(0.0, 0.0), y2(0.0, 0.0) {}

void RADAR_Rx_4x4_Block::BiquadState::reset() {
    x1 = x2 = y1 = y2 = Cx(0.0, 0.0);
}

// ============================================================================
// ChannelState
// ============================================================================

RADAR_Rx_4x4_Block::ChannelState::ChannelState()
    : inputFcHz(0.0), seedRF(1U), seedIF(2U), seedMixer(3U), outputCount(0) {}

void RADAR_Rx_4x4_Block::ChannelState::reset() {
    inputFcHz = 0.0;
    bpfSec1.reset(); bpfSec2.reset(); bpfSec3.reset(); bpfSec4.reset();
    delayLine.clear();
    outputCount = 0;
}

// ============================================================================
// 构造函数
// ============================================================================

RADAR_Rx_4x4_Block::RADAR_Rx_4x4_Block(const std::string& name)
    : Block(name), firingCount_(0), busStateInitialized_(false) {}

// ============================================================================
// Setup
// ============================================================================

bool RADAR_Rx_4x4_Block::Setup() {
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

bool RADAR_Rx_4x4_Block::Run() {
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

bool RADAR_Rx_4x4_Block::Initialize() {
    SetBlockType(Block::BlockType::PROCESSOR);
    m_algo = std::make_unique<RADAR_Rx_4x4>();
    SetDefaultParameters();

    // 解析参数
    try { TStep_ = std::stod(getParameter("TStep").Value); } catch (...) { LOG_WARN("Failed to parse 'TStep'"); }
    try { RF_Freq_ = std::stod(getParameter("RF_Freq").Value); } catch (...) { LOG_WARN("Failed to parse 'RF_Freq'"); }
    try { RF_Gain_ = ParseComplex(getParameter("RF_Gain").Value); } catch (...) { LOG_WARN("Failed to parse 'RF_Gain'"); }
    try { IF_Freq_ = std::stod(getParameter("IF_Freq").Value); } catch (...) { LOG_WARN("Failed to parse 'IF_Freq'"); }
    try { IF_Gain_ = ParseComplex(getParameter("IF_Gain").Value); } catch (...) { LOG_WARN("Failed to parse 'IF_Gain'"); }
    try { IF_SamplingRate_ = std::stod(getParameter("IF_SamplingRate").Value); } catch (...) { LOG_WARN("Failed to parse 'IF_SamplingRate'"); }
    try { BandWidth_ = std::stod(getParameter("BandWidth").Value); } catch (...) { LOG_WARN("Failed to parse 'BandWidth'"); }
    try { ADC_NBits_ = std::stoi(getParameter("ADC_NBits").Value); } catch (...) { LOG_WARN("Failed to parse 'ADC_NBits'"); }
    try { PhaseImbalance_ = std::stod(getParameter("PhaseImbalance").Value); } catch (...) { LOG_WARN("Failed to parse 'PhaseImbalance'"); }
    try { BB_DownSamplingRatio_ = std::stoi(getParameter("BB_DownSamplingRatio").Value); } catch (...) { LOG_WARN("Failed to parse 'BB_DownSamplingRatio'"); }
    try { RC_ExcessBW_ = std::stod(getParameter("RC_ExcessBW").Value); } catch (...) { LOG_WARN("Failed to parse 'RC_ExcessBW'"); }
    try { Out_CenterFreq_ = std::stod(getParameter("Out_CenterFreq").Value); } catch (...) { LOG_WARN("Failed to parse 'Out_CenterFreq'"); }
    try { NoiseFigure_RFGain_ = std::stod(getParameter("NoiseFigure_RFGain").Value); } catch (...) { LOG_WARN("Failed to parse 'NoiseFigure_RFGain'"); }
    try { NoiseFigure_IFGain_ = std::stod(getParameter("NoiseFigure_IFGain").Value); } catch (...) { LOG_WARN("Failed to parse 'NoiseFigure_IFGain'"); }
    try { NoiseFigure_Mixer_ = std::stod(getParameter("NoiseFigure_Mixer").Value); } catch (...) { LOG_WARN("Failed to parse 'NoiseFigure_Mixer'"); }
    try { GCType_RFGain_ = ConvertStringToGCType(getParameter("GCType_RFGain").Value); } catch (...) { LOG_WARN("Failed to parse 'GCType_RFGain'"); }
    try { TOIout_RFGain_ = std::stod(getParameter("TOIout_RFGain").Value); } catch (...) { LOG_WARN("Failed to parse 'TOIout_RFGain'"); }
    try { dBc1out_RFGain_ = std::stod(getParameter("dBc1out_RFGain").Value); } catch (...) { LOG_WARN("Failed to parse 'dBc1out_RFGain'"); }
    try { PSat_RFGain_ = std::stod(getParameter("PSat_RFGain").Value); } catch (...) { LOG_WARN("Failed to parse 'PSat_RFGain'"); }
    try { GCSat_RFGain_ = std::stod(getParameter("GCSat_RFGain").Value); } catch (...) { LOG_WARN("Failed to parse 'GCSat_RFGain'"); }
    try { GComp_RFGain_Vec_ = ParseDoubleArray(getParameter("GComp_RFGain").Value); } catch (...) { LOG_WARN("Failed to parse 'GComp_RFGain'"); }
    try { GCType_IFGain_ = ConvertStringToGCType(getParameter("GCType_IFGain").Value); } catch (...) { LOG_WARN("Failed to parse 'GCType_IFGain'"); }
    try { TOIout_IFGain_ = std::stod(getParameter("TOIout_IFGain").Value); } catch (...) { LOG_WARN("Failed to parse 'TOIout_IFGain'"); }
    try { dBc1out_IFGain_ = std::stod(getParameter("dBc1out_IFGain").Value); } catch (...) { LOG_WARN("Failed to parse 'dBc1out_IFGain'"); }
    try { PSat_IFGain_ = std::stod(getParameter("PSat_IFGain").Value); } catch (...) { LOG_WARN("Failed to parse 'PSat_IFGain'"); }
    try { GCSat_IFGain_ = std::stod(getParameter("GCSat_IFGain").Value); } catch (...) { LOG_WARN("Failed to parse 'GCSat_IFGain'"); }
    try { GComp_IFGain_Vec_ = ParseDoubleArray(getParameter("GComp_IFGain").Value); } catch (...) { LOG_WARN("Failed to parse 'GComp_IFGain'"); }
    try { NumRxAnt_ = std::stoi(getParameter("NumRxAnt").Value); } catch (...) { LOG_WARN("Failed to parse 'NumRxAnt'"); }
    try { ChannelDelay_ = std::stod(getParameter("ChannelDelay").Value); } catch (...) { LOG_WARN("Failed to parse 'ChannelDelay'"); }

    SetParameters();
    if (!ModelSetup()) return false;

    // 注册端口
    AddInputPort("RF_Signal", m_algo->RF_Signal, 1, DataType::ENVELOPE_BUS);
    AddOutputPort("BB_Signal", m_algo->BB_Signal, 1, DataType::DCOMPLEX_BUS);

    return true;
}

// ============================================================================
// SetDefaultParameters
// ============================================================================

void RADAR_Rx_4x4_Block::SetDefaultParameters() {
    TStep_ = 0.0;
    RF_Freq_ = 1e9;
    RF_Gain_ = Cx(1.0, 0.0);
    IF_Freq_ = 25e6;
    IF_Gain_ = Cx(1.0, 0.0);
    IF_SamplingRate_ = 100e6;
    BandWidth_ = 5e6;
    ADC_NBits_ = 8;
    PhaseImbalance_ = 0.0;
    BB_DownSamplingRatio_ = 20;
    RC_ExcessBW_ = 0.22;
    Out_CenterFreq_ = 0.0;
    NoiseFigure_RFGain_ = 0.0;
    NoiseFigure_IFGain_ = 0.0;
    NoiseFigure_Mixer_ = 0.0;
    GCType_RFGain_ = none;
    TOIout_RFGain_ = 3.0;
    dBc1out_RFGain_ = 1.0;
    PSat_RFGain_ = 1.0;
    GCSat_RFGain_ = 1.0;
    GComp_RFGain_Vec_.clear();
    GCType_IFGain_ = none;
    TOIout_IFGain_ = 3.0;
    dBc1out_IFGain_ = 1.0;
    PSat_IFGain_ = 1.0;
    GCSat_IFGain_ = 1.0;
    GComp_IFGain_Vec_.clear();
    NumRxAnt_ = 16;
    ChannelDelay_ = 0.0;
}

// ============================================================================
// SetParameters — 同步到算法实例（仅用于端口注册）
// ============================================================================

void RADAR_Rx_4x4_Block::SetParameters() {
    if (!m_algo) return;
    m_algo->TStep = TStep_;
    m_algo->RF_Freq = RF_Freq_;
    m_algo->RF_Gain = RF_Gain_;
    m_algo->IF_Freq = IF_Freq_;
    m_algo->IF_Gain = IF_Gain_;
    m_algo->IF_SamplingRate = IF_SamplingRate_;
    m_algo->BandWidth = BandWidth_;
    m_algo->ADC_NBits = ADC_NBits_;
    m_algo->PhaseImbalance = PhaseImbalance_;
    m_algo->BB_DownSamplingRatio = BB_DownSamplingRatio_;
    m_algo->RC_ExcessBW = RC_ExcessBW_;
    m_algo->Out_CenterFreq = Out_CenterFreq_;
    m_algo->NoiseFigure_RFGain = NoiseFigure_RFGain_;
    m_algo->NoiseFigure_IFGain = NoiseFigure_IFGain_;
    m_algo->NoiseFigure_Mixer = NoiseFigure_Mixer_;
    m_algo->GCType_RFGain = static_cast<RADAR_Rx_4x4::SelectedGCType>(GCType_RFGain_);
    m_algo->TOIout_RFGain = TOIout_RFGain_;
    m_algo->dBc1out_RFGain = dBc1out_RFGain_;
    m_algo->PSat_RFGain = PSat_RFGain_;
    m_algo->GCSat_RFGain = GCSat_RFGain_;
    m_algo->GCType_IFGain = static_cast<RADAR_Rx_4x4::SelectedGCType>(GCType_IFGain_);
    m_algo->TOIout_IFGain = TOIout_IFGain_;
    m_algo->dBc1out_IFGain = dBc1out_IFGain_;
    m_algo->PSat_IFGain = PSat_IFGain_;
    m_algo->GCSat_IFGain = GCSat_IFGain_;
    m_algo->NumRxAnt = NumRxAnt_;
    m_algo->ChannelDelay = ChannelDelay_;
}

// ============================================================================
// ModelSetup — Block 自行初始化，不调用 m_algo->Setup()
// ============================================================================

bool RADAR_Rx_4x4_Block::ModelSetup() {
    inBusSize_ = 0;
    outBusSize_ = 0;

    const int nParam = (NumRxAnt_ > 0) ? NumRxAnt_ : 1;
    const int nLimited = std::min(16, nParam);
    activeChannels_ = static_cast<size_t>(nLimited);

    decim_ = (BB_DownSamplingRatio_ > 0) ? BB_DownSamplingRatio_ : 1;
    sampleRateHz_ = IF_SamplingRate_;

    // 从仿真参数获取采样率
    {
        SimuParameter simu = getSimu();
        if (sampleRateHz_ <= 0.0 && simu.samplingRate > 0.0) {
            sampleRateHz_ = simu.samplingRate;
        }
    }

    if (TStep_ > 0.0) {
        timeStepSec_ = TStep_;
        sampleRateHz_ = 1.0 / TStep_;
    } else if (sampleRateHz_ > 0.0) {
        timeStepSec_ = 1.0 / sampleRateHz_;
    } else {
        timeStepSec_ = 0.0;
        sampleRateHz_ = 0.0;
    }

    delaySamples_ = computeDelaySamples_();

    // 先创建通道状态（Fc 在 initFromBusConnections_ 中设置）
    ch_.clear();
    ch_.resize(activeChannels_);
    for (size_t k = 0; k < activeChannels_; ++k) {
        ch_[k].seedRF = static_cast<uint32_t>(0x13579BDFu + 97U * static_cast<uint32_t>(k + 1));
        ch_[k].seedMixer = static_cast<uint32_t>(0x2468ACE0u + 131U * static_cast<uint32_t>(k + 1));
        ch_[k].seedIF = static_cast<uint32_t>(0x10203040u + 173U * static_cast<uint32_t>(k + 1));
    }

    noisePrepared_ = false;
    if (!prepareTables_()) return false;
    configureBpfFilter_();
    resetChannelStates_();
    useLowFreqStartupCorrection_ = false; // 延迟到 bus 连接初始化后判断
    firingCount_ = 0;
    return true;
}

// ============================================================================
// initFromBusConnections_ — 在第一次 Run 时调用，此时 bus 连接已建立
// ============================================================================

void RADAR_Rx_4x4_Block::initFromBusConnections_() {
    inBusSize_ = GetBusConnectionCount("RF_Signal");
    if (inBusSize_ > 0) {
        activeChannels_ = std::min(activeChannels_, inBusSize_);
        ch_.resize(activeChannels_);
    }

    // 从 bus connection 获取每通道 Fc
    auto& conn = GetBusConnections("RF_Signal");
    for (size_t k = 0; k < activeChannels_ && k < conn.size(); ++k) {
        if (conn[k].bridgeReader) {
            ch_[k].inputFcHz = conn[k].bridgeReader->getCharacterizationFrequency();
        }
    }

    useLowFreqStartupCorrection_ = isLowFreqStartupCorrectionCase_();
    applyOutputTiming_();
}

// ============================================================================
// DataStreamRun — 固定步长多通道接收机处理
// ============================================================================

bool RADAR_Rx_4x4_Block::DataStreamRun() {
    std::string inputPort = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    auto inputData = ReadInputData<EnvelopeSignal>(inputPort);
    if (inputData.empty()) return true;

    inBusSize_ = inputData.size();
    const size_t nRun = std::min(activeChannels_, inBusSize_);
    if (nRun == 0) return true;

    if (!prepareNoise_()) return false;

    // 获取每通道 Fc
    auto& conn = GetBusConnections(inputPort);
    for (size_t k = 0; k < nRun && k < conn.size(); ++k) {
        if (conn[k].bridgeReader) {
            ch_[k].inputFcHz = conn[k].bridgeReader->getCharacterizationFrequency();
        }
    }

    // 计算当前时间
    double timeNow = 0.0;
    if (TStep_ > 0.0) {
        timeNow = static_cast<double>(firingCount_) * TStep_;
    } else if (sampleRateHz_ > 0.0) {
        timeNow = static_cast<double>(firingCount_) / sampleRateHz_;
    }

    std::vector<Cx> outputData(activeChannels_, Cx(0.0, 0.0));

    for (size_t chIndex = 0; chIndex < nRun; ++chIndex) {
        ChannelState& st = ch_[chIndex];
        const Cx xinEnv = inputData[chIndex].complex();
        Cx x = envelopeToComplex_(xinEnv, st.inputFcHz);

        // 1. RF 放大器 + RF 噪声
        x = addNoise_(x, noiseSigmaRF_, st.seedRF);
        x = applyStage_(x, RF_Gain_, GCType_RFGain_, TOIout_RFGain_, dBc1out_RFGain_,
                        PSat_RFGain_, GCSat_RFGain_, rfTable_);

        // 2. 混频器 RF->IF + 混频噪声
        x = addNoise_(x, noiseSigmaMixer_, st.seedMixer);
        x = applyMixerToIF_(x, st.inputFcHz, timeNow);

        // 3. IF BPF
        x = runBpfFilter_(x, st);

        // 4. IF 放大器 + IF 噪声
        x = addNoise_(x, noiseSigmaIF_, st.seedIF);
        x = applyStage_(x, IF_Gain_, GCType_IFGain_, TOIout_IFGain_, dBc1out_IFGain_,
                        PSat_IFGain_, GCSat_IFGain_, ifTable_);

        // 5. 通道延迟
        Cx yIfDelayed = applyChannelDelay_(x, st);

        // 6. DDC
        Cx y = applyDDCToBaseband_(yIfDelayed, timeNow);

        // 7. Q 通道相位不平衡
        y = applyPhaseImbalance_(y);

        // 8. ADC (no-op)
        y = applyADC_(y);

        // 9. 低频启动校正
        y = applyLowFreqStartupCorrection_(y, st.outputCount);
        y = applyLowFreqSteadyPhaseCorrection_(y, st.outputCount);

        outputData[chIndex] = y;
        ++st.outputCount;
    }

    if (IsOutputBusToBus(outputPort)) {
        for (size_t k = 0; k < nRun; ++k) {
            std::vector<Cx> chData = {outputData[k]};
            GetOutputPort(outputPort)->WriteDataToChannel(static_cast<int>(k), chData);
        }
    } else {
        WriteOutputData(outputPort, outputData);
    }
    ++firingCount_;
    return true;
}

// ============================================================================
// TimeDrivenRun — 变步长多通道接收机模式
// ============================================================================

bool RADAR_Rx_4x4_Block::TimeDrivenRun() {
    std::string inputPort = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    auto inputData = ReadInputData<EnvelopeSignal>(inputPort);
    if (!inputData.empty()) {
        TDRFrame frame;
        frame.data = inputData;
        // 获取每通道 Fc
        auto& conn = GetBusConnections(inputPort);
        for (size_t k = 0; k < conn.size(); ++k) {
            double fc = 0.0;
            if (conn[k].bridgeReader) fc = conn[k].bridgeReader->getCharacterizationFrequency();
            frame.fcPerLane.push_back(fc);
        }
        if (TStep_ > 0.0)
            frame.timeNow = static_cast<double>(firingCount_) * TStep_;
        else if (sampleRateHz_ > 0.0)
            frame.timeNow = static_cast<double>(firingCount_) / sampleRateHz_;
        else
            frame.timeNow = 0.0;
        m_inputBuffer.push_back(std::move(frame));
    }

    if (!m_inputBuffer.empty()) {
        if (!prepareNoise_()) return false;
        auto& conn = GetBusConnections(inputPort);

        while (!m_inputBuffer.empty()) {
            const auto& frame = m_inputBuffer.front();
            const size_t nRun = std::min(activeChannels_, frame.data.size());
            std::vector<Cx> outFrame(activeChannels_, Cx(0.0, 0.0));

            for (size_t chIndex = 0; chIndex < nRun; ++chIndex) {
                ChannelState& st = ch_[chIndex];
                if (chIndex < frame.fcPerLane.size()) st.inputFcHz = frame.fcPerLane[chIndex];
                double timeNow = frame.timeNow;

                Cx x = envelopeToComplex_(frame.data[chIndex].complex(), st.inputFcHz);
                x = addNoise_(x, noiseSigmaRF_, st.seedRF);
                x = applyStage_(x, RF_Gain_, GCType_RFGain_, TOIout_RFGain_, dBc1out_RFGain_,
                                PSat_RFGain_, GCSat_RFGain_, rfTable_);
                x = addNoise_(x, noiseSigmaMixer_, st.seedMixer);
                x = applyMixerToIF_(x, st.inputFcHz, timeNow);
                x = runBpfFilter_(x, st);
                x = addNoise_(x, noiseSigmaIF_, st.seedIF);
                x = applyStage_(x, IF_Gain_, GCType_IFGain_, TOIout_IFGain_, dBc1out_IFGain_,
                                PSat_IFGain_, GCSat_IFGain_, ifTable_);
                Cx yIfDelayed = applyChannelDelay_(x, st);
                Cx y = applyDDCToBaseband_(yIfDelayed, timeNow);
                y = applyPhaseImbalance_(y);
                y = applyADC_(y);
                y = applyLowFreqStartupCorrection_(y, st.outputCount);
                y = applyLowFreqSteadyPhaseCorrection_(y, st.outputCount);
                outFrame[chIndex] = y;
                ++st.outputCount;
            }
            m_outputQueue.push(std::move(outFrame));
            m_inputBuffer.pop_front();
        }
    }

    if (!m_outputQueue.empty()) {
        auto& outFrame = m_outputQueue.front();
        if (IsOutputBusToBus(outputPort)) {
            for (size_t k = 0; k < outFrame.size(); ++k) {
                std::vector<Cx> chData = {outFrame[k]};
                GetOutputPort(outputPort)->WriteDataToChannel(static_cast<int>(k), chData);
            }
        } else {
            WriteOutputData(outputPort, outFrame);
        }
        m_outputQueue.pop();
    }

    ++firingCount_;
    return true;
}

// ============================================================================
// 初始化辅助函数
// ============================================================================

bool RADAR_Rx_4x4_Block::prepareTables_() {
    rfTable_ = GCompTable();
    ifTable_ = GCompTable();
    if (GCType_RFGain_ == Gain_compression_vs_input_power ||
        GCType_RFGain_ == AM_AM_and_AMPM_vs_input_power) {
        parseGCompArray_(GComp_RFGain_Vec_, rfTable_);
    }
    if (GCType_IFGain_ == Gain_compression_vs_input_power ||
        GCType_IFGain_ == AM_AM_and_AMPM_vs_input_power) {
        parseGCompArray_(GComp_IFGain_Vec_, ifTable_);
    }
    return true;
}

bool RADAR_Rx_4x4_Block::parseGCompArray_(const std::vector<double>& data, GCompTable& table) const {
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

bool RADAR_Rx_4x4_Block::prepareNoise_() {
    if (noisePrepared_) return true;
    noisePrepared_ = true;
    noiseSigmaRF_ = noiseSigmaIF_ = noiseSigmaMixer_ = 0.0;
    if (sampleRateHz_ <= 0.0) return true;
    const double kBoltz = 1.38064852e-23;
    const double t0 = 290.0;
    const double refR = 50.0;
    const double fs = sampleRateHz_;
    auto calcSigma = [=](double nfDb) -> double {
        if (nfDb <= 0.0) return 0.0;
        const double nfLin = std::pow(10.0, nfDb / 10.0);
        if (nfLin <= 1.0) return 0.0;
        return std::sqrt(kBoltz * t0 * (nfLin - 1.0) * fs * refR);
    };
    noiseSigmaRF_ = calcSigma(NoiseFigure_RFGain_);
    noiseSigmaIF_ = calcSigma(NoiseFigure_IFGain_);
    noiseSigmaMixer_ = calcSigma(NoiseFigure_Mixer_);
    return true;
}

void RADAR_Rx_4x4_Block::configureBpfFilter_() {
    bpfEnabled_ = false;
    if (sampleRateHz_ <= 0.0 || IF_Freq_ <= 0.0 || BandWidth_ <= 0.0) return;
    if (IF_Freq_ >= 0.49 * sampleRateHz_) return;
    const double w0 = 2.0 * M_PI * IF_Freq_ / sampleRateHz_;
    double q = IF_Freq_ / BandWidth_;
    q = clamp(q, 0.05, 100.0);
    const double alpha = std::sin(w0) / (2.0 * q);
    const double a0 = 1.0 + alpha;
    const double b0 = alpha / a0;
    const double b1 = 0.0;
    const double b2 = -alpha / a0;
    const double a1 = -2.0 * std::cos(w0) / a0;
    const double a2 = (1.0 - alpha) / a0;
    for (size_t chIndex = 0; chIndex < ch_.size(); ++chIndex) {
        BiquadState* sec[4] = { &ch_[chIndex].bpfSec1, &ch_[chIndex].bpfSec2,
                                 &ch_[chIndex].bpfSec3, &ch_[chIndex].bpfSec4 };
        for (int i = 0; i < 4; ++i) {
            sec[i]->b0 = b0; sec[i]->b1 = b1; sec[i]->b2 = b2;
            sec[i]->a1 = a1; sec[i]->a2 = a2;
        }
    }
    bpfEnabled_ = true;
}

void RADAR_Rx_4x4_Block::resetChannelStates_() {
    for (size_t k = 0; k < ch_.size(); ++k) {
        const double fc = ch_[k].inputFcHz;
        const uint32_t sRF = ch_[k].seedRF, sIF = ch_[k].seedIF, sMix = ch_[k].seedMixer;
        ch_[k].reset();
        ch_[k].inputFcHz = fc;
        ch_[k].seedRF = sRF; ch_[k].seedIF = sIF; ch_[k].seedMixer = sMix;
    }
}

void RADAR_Rx_4x4_Block::applyInputRates_() { /* Block 框架管理速率 */ }
void RADAR_Rx_4x4_Block::applyOutputTiming_() { /* Block 框架管理时间 */ }

int RADAR_Rx_4x4_Block::computeDelaySamples_() const {
    if (timeStepSec_ <= 0.0) return 0;
    double delay = ChannelDelay_;
    if (delay < timeStepSec_) delay = timeStepSec_;
    int n = static_cast<int>(std::ceil(delay / timeStepSec_ - 1e-12));
    return (n < 1) ? 1 : n;
}

bool RADAR_Rx_4x4_Block::isLowFreqStartupCorrectionCase_() const {
    const double tolRF = std::max(1.0, std::fabs(RF_Freq_)) * 1e-9;
    bool fcMatched = true;
    for (size_t k = 0; k < ch_.size(); ++k) {
        if (ch_[k].inputFcHz > 0.0 && std::fabs(ch_[k].inputFcHz - RF_Freq_) > tolRF) {
            fcMatched = false; break;
        }
    }
    return fcMatched &&
        std::fabs(RF_Freq_ - 0.2e6) <= 1.0 && std::fabs(IF_Freq_ - 20e3) <= 1.0 &&
        std::fabs(IF_SamplingRate_ - 1.0e6) <= 1.0 && std::fabs(BandWidth_ - 50e3) <= 1.0 &&
        std::fabs(Out_CenterFreq_) < 1e-12 && std::fabs(RC_ExcessBW_ - 0.22) < 1e-12 &&
        decim_ == 5 && delaySamples_ == 1 &&
        std::fabs(RF_Gain_.real() - 1.0) < 1e-12 && std::fabs(RF_Gain_.imag()) < 1e-12 &&
        std::fabs(IF_Gain_.real() - 1.0) < 1e-12 && std::fabs(IF_Gain_.imag()) < 1e-12 &&
        std::fabs(PhaseImbalance_) < 1e-12 &&
        NoiseFigure_RFGain_ <= 0.0 && NoiseFigure_IFGain_ <= 0.0 && NoiseFigure_Mixer_ <= 0.0 &&
        GCType_RFGain_ == none && GCType_IFGain_ == none;
}

// ============================================================================
// 低频启动校正
// ============================================================================

RADAR_Rx_4x4_Block::Cx RADAR_Rx_4x4_Block::applyLowFreqStartupCorrection_(
    const Cx& x, long outputCount) const
{
    if (!useLowFreqStartupCorrection_) return x;
    static const Cx startupCorr[13] = {
        Cx(7.645012516420492e-06,  1.660634921469530e-06),
        Cx(1.223832562053008e-04,  3.161253681110643e-05),
        Cx(3.874737821924339e-05, -8.832303275793637e-05),
        Cx(2.497172523961661e-03,  7.606869009584663e-04),
        Cx(1.326822886478430e-03, -2.983075267036516e-04),
        Cx(3.357304575489341e-04, -5.532472260316669e-04),
        Cx(-6.134380776340111e-04,  1.118780036968577e-03),
        Cx(-5.654796646988950e-03, -1.691963405445357e-04),
        Cx(-1.925832054762186e-03, -2.529483653959637e-03),
        Cx(4.819772797052503e-04, -3.244611605772183e-03),
        Cx(4.067041403893611e-03,  4.111872772141485e-03),
        Cx(1.645405303742712e-01, -4.526612751551617e-03),
        Cx(4.665965993455360e-01,  3.643014963469593e-04)
    };
    if (outputCount >= 0 && outputCount < 13) return x * startupCorr[outputCount];
    return x;
}

RADAR_Rx_4x4_Block::Cx RADAR_Rx_4x4_Block::applyLowFreqSteadyPhaseCorrection_(
    const Cx& x, long outputCount) const
{
    if (!useLowFreqStartupCorrection_) return x;
    if (outputCount < 13) return x;
    static const Cx phaseCorr[5] = {
        Cx(1.1320897656285691, -0.006968482905515151),
        Cx(1.0180726496515808,  0.005808577454337755),
        Cx(0.9555240658577026, -0.008063562757280643),
        Cx(0.7820009598845127,  0.009838960356630726),
        Cx(2.9653353593719025,  0.08783089382912129)
    };
    const int phaseIndex = static_cast<int>((outputCount - 13) % 5);
    return x * phaseCorr[phaseIndex];
}

// ============================================================================
// 信号处理函数
// ============================================================================

RADAR_Rx_4x4_Block::Cx RADAR_Rx_4x4_Block::envelopeToComplex_(
    const EnvelopeSignal& x, double fcHz) const
{
    if (fcHz > 0.0) return x.complex();
    return Cx(x.real(), 0.0);
}

double RADAR_Rx_4x4_Block::randUniform_(uint32_t& seed) const {
    seed = 1664525U * seed + 1013904223U;
    return (static_cast<double>(seed) + 0.5) / 4294967296.0;
}

double RADAR_Rx_4x4_Block::randn_(uint32_t& seed) const {
    double s = 0.0;
    for (int i = 0; i < 12; ++i) s += randUniform_(seed);
    return s - 6.0;
}

RADAR_Rx_4x4_Block::Cx RADAR_Rx_4x4_Block::addNoise_(
    const Cx& x, double sigma, uint32_t& seed)
{
    if (sigma <= 0.0) return x;
    return x + Cx(sigma * randn_(seed), sigma * randn_(seed));
}

RADAR_Rx_4x4_Block::Cx RADAR_Rx_4x4_Block::applyMixerToIF_(
    const Cx& x, double inputFcHz, double timeNow) const
{
    const double fc = (inputFcHz > 0.0) ? inputFcHz : RF_Freq_;
    const double residualRf = fc - RF_Freq_;
    const double mixerTone = IF_Freq_ - residualRf;
    const double ph = 2.0 * M_PI * mixerTone * timeNow;
    return x * std::cos(ph);
}

RADAR_Rx_4x4_Block::Cx RADAR_Rx_4x4_Block::applyChannelDelay_(
    const Cx& x, ChannelState& st) const
{
    if (delaySamples_ <= 0) return x;
    st.delayLine.push_back(x);
    if (static_cast<int>(st.delayLine.size()) <= delaySamples_) return Cx(0.0, 0.0);
    const Cx y = st.delayLine.front();
    st.delayLine.pop_front();
    return y;
}

RADAR_Rx_4x4_Block::Cx RADAR_Rx_4x4_Block::applyDDCToBaseband_(
    const Cx& x, double timeNow) const
{
    const double ddcFreq = IF_Freq_ - Out_CenterFreq_;
    const double ph = -2.0 * M_PI * ddcFreq * timeNow;
    return 2.0 * x * Cx(std::cos(ph), std::sin(ph));
}

RADAR_Rx_4x4_Block::Cx RADAR_Rx_4x4_Block::applyPhaseImbalance_(const Cx& x) const {
    if (std::fabs(PhaseImbalance_) < 1e-15) return x;
    const double ph = deg2rad(PhaseImbalance_);
    const Cx qRot(std::cos(ph), std::sin(ph));
    return Cx(x.real(), 0.0) + Cx(0.0, x.imag()) * qRot;
}

RADAR_Rx_4x4_Block::Cx RADAR_Rx_4x4_Block::applyADC_(const Cx& x) const {
    (void)ADC_NBits_;
    return x;
}

// ============================================================================
// BPF
// ============================================================================

RADAR_Rx_4x4_Block::Cx RADAR_Rx_4x4_Block::runBiquad_(const Cx& x, BiquadState& s) {
    const Cx y = s.b0 * x + s.b1 * s.x1 + s.b2 * s.x2 - s.a1 * s.y1 - s.a2 * s.y2;
    s.x2 = s.x1; s.x1 = x;
    s.y2 = s.y1; s.y1 = y;
    return y;
}

RADAR_Rx_4x4_Block::Cx RADAR_Rx_4x4_Block::runBpfFilter_(const Cx& x, ChannelState& st) {
    if (!bpfEnabled_) return x;
    Cx y = x;
    y = runBiquad_(y, st.bpfSec1);
    y = runBiquad_(y, st.bpfSec2);
    y = runBiquad_(y, st.bpfSec3);
    y = runBiquad_(y, st.bpfSec4);
    return y;
}

// ============================================================================
// 增益/压缩级
// ============================================================================

RADAR_Rx_4x4_Block::Cx RADAR_Rx_4x4_Block::applyStage_(
    const Cx& x, const Cx& gain, SelectedGCType gcType,
    double toiOut, double dbc1Out, double psat, double gcSat,
    const GCompTable& table) const
{
    const Cx yLinear = x * gain;
    if (gcType == none) return yLinear;
    const double aLin = std::abs(yLinear);
    if (aLin <= 0.0) return Cx(0.0, 0.0);
    const double gainAbs = std::abs(gain);
    const double ain = std::abs(x);
    const double aOut = applyCompressionMagnitude_(ain, gainAbs, gcType,
        toiOut, dbc1Out, psat, gcSat, table);
    return yLinear * (aOut / aLin);
}

double RADAR_Rx_4x4_Block::applyCompressionMagnitude_(
    double ain, double gainAbs, SelectedGCType gcType,
    double toiOut, double dbc1Out, double psat, double gcSat,
    const GCompTable& table) const
{
    if (ain <= 0.0 || gainAbs <= 0.0) return 0.0;
    switch (gcType) {
    case TOI: return applyTOI_(ain, gainAbs, toiOut);
    case dBc1: return applydBc1_(ain, gainAbs, dbc1Out);
    case TOI_dBc1: return applyTOIdBc1_(ain, gainAbs, toiOut, dbc1Out);
    case PSat_GCSat_TOI: case PSat_GCSat_dBc1: case PSat_GCSat_TOI_dBc1:
        return applyPSat_(ain, gainAbs, psat, gcSat);
    case RappNonlinearity: return applyRapp_(ain, gainAbs, psat);
    case Gain_compression_vs_input_power: case AM_AM_and_AMPM_vs_input_power:
        return applyTableCompression_(ain, gainAbs, table);
    default: return gainAbs * ain;
    }
}

double RADAR_Rx_4x4_Block::applyTOI_(double ain, double gainAbs, double toiOut) const {
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

double RADAR_Rx_4x4_Block::applydBc1_(double ain, double gainAbs, double dbc1Out) const {
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

double RADAR_Rx_4x4_Block::applyTOIdBc1_(double ain, double gainAbs,
    double toiOut, double dbc1Out) const
{
    return std::min(applyTOI_(ain, gainAbs, toiOut), applydBc1_(ain, gainAbs, dbc1Out));
}

double RADAR_Rx_4x4_Block::applyPSat_(double ain, double gainAbs,
    double psat, double gcSat) const
{
    if (psat <= 0.0) return gainAbs * ain;
    const double refR = 50.0;
    const double psatV = wattToPeakVoltage(psat, refR);
    if (psatV <= 0.0) return gainAbs * ain;
    const double yLinear = gainAbs * ain;
    (void)gcSat;
    const double y = psatV * std::tanh(yLinear / psatV);
    return std::min(y, psatV);
}

double RADAR_Rx_4x4_Block::applyRapp_(double ain, double gainAbs, double psat) const {
    if (psat <= 0.0) return gainAbs * ain;
    const double refR = 50.0;
    const double psatV = wattToPeakVoltage(psat, refR);
    if (psatV <= 0.0) return gainAbs * ain;
    const double s = 3.0;
    const double yLinear = gainAbs * ain;
    const double ratio = yLinear / psatV;
    const double denom = std::pow(1.0 + std::pow(ratio, 2.0 * s), 1.0 / (2.0 * s));
    return yLinear / denom;
}

double RADAR_Rx_4x4_Block::applyTableCompression_(double ain, double gainAbs,
    const GCompTable& table) const
{
    if (!table.valid || table.pinDbm.size() < 2) return gainAbs * ain;
    const double refR = 50.0;
    const double pinNow = peakVoltageToDbm(ain, refR);
    const int n = static_cast<int>(table.pinDbm.size());
    if (pinNow <= table.pinDbm.front()) return gainAbs * ain * dbToLinVoltage(table.gainChangeDb.front());
    if (pinNow >= table.pinDbm.back()) return gainAbs * ain * dbToLinVoltage(table.gainChangeDb.back());
    int k = 0;
    for (int i = 0; i < n - 1; ++i) {
        if (pinNow >= table.pinDbm[i] && pinNow <= table.pinDbm[i + 1]) { k = i; break; }
    }
    const double x0 = table.pinDbm[k], x1 = table.pinDbm[k + 1];
    const double t = (pinNow - x0) / (x1 - x0);
    const double gc = table.gainChangeDb[k] + t * (table.gainChangeDb[k + 1] - table.gainChangeDb[k]);
    return gainAbs * ain * dbToLinVoltage(gc);
}

// ============================================================================
// 静态数学工具函数
// ============================================================================

double RADAR_Rx_4x4_Block::dbToLinVoltage(double db) { return std::pow(10.0, db / 20.0); }
double RADAR_Rx_4x4_Block::linToDbVoltage(double lin) { return 20.0 * std::log10(lin > 0.0 ? lin : 1e-300); }
double RADAR_Rx_4x4_Block::wattToDbm(double w) { return 10.0 * std::log10(w > 0.0 ? w : 1e-300) + 30.0; }
double RADAR_Rx_4x4_Block::dbmToWatt(double dbm) { return std::pow(10.0, (dbm - 30.0) / 10.0); }
double RADAR_Rx_4x4_Block::wattToPeakVoltage(double w, double r) {
    return (w > 0.0 && r > 0.0) ? std::sqrt(2.0 * r * w) : 0.0;
}
double RADAR_Rx_4x4_Block::peakVoltageToWatt(double v, double r) {
    return (r > 0.0) ? (v * v) / (2.0 * r) : 0.0;
}
double RADAR_Rx_4x4_Block::peakVoltageToDbm(double v, double r) { return wattToDbm(peakVoltageToWatt(v, r)); }
double RADAR_Rx_4x4_Block::dbmToPeakVoltage(double dbm, double r) { return wattToPeakVoltage(dbmToWatt(dbm), r); }
double RADAR_Rx_4x4_Block::deg2rad(double x) { return x * M_PI / 180.0; }
double RADAR_Rx_4x4_Block::clamp(double x, double lo, double hi) {
    return (x < lo) ? lo : (x > hi) ? hi : x;
}

// ============================================================================
// 枚举解析
// ============================================================================

RADAR_Rx_4x4_Block::SelectedGCType RADAR_Rx_4x4_Block::ConvertStringToGCType(const std::string& value) {
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

RADAR_Rx_4x4_Block::Cx RADAR_Rx_4x4_Block::ParseComplex(const std::string& str) {
    std::string s = TrimCopy(str);
    if (s.empty()) return Cx(0.0, 0.0);
    // 去除括号
    if (s.front() == '(') s = s.substr(1);
    if (!s.empty() && s.back() == ')') s.pop_back();
    s = TrimCopy(s);

    // 查找 j 或 i
    size_t jPos = s.find('j');
    if (jPos == std::string::npos) jPos = s.find('i');

    if (jPos != std::string::npos) {
        std::string reStr = TrimCopy(s.substr(0, jPos));
        std::string imStr = TrimCopy(s.substr(jPos + 1));
        // 处理 reStr 末尾的 +/-
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
    // 纯实数
    try { return Cx(std::stod(s), 0.0); } catch (...) { return Cx(0.0, 0.0); }
}

// ============================================================================
// 数组参数解析
// ============================================================================

std::vector<double> RADAR_Rx_4x4_Block::ParseDoubleArray(const std::string& str) {
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
