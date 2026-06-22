#include "RADAR_DUC_Block.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================================================
// BiquadState
// ============================================================

void RADAR_DUC_Block::BiquadState::reset()
{
    x1 = std::complex<double>(0.0, 0.0);
    x2 = std::complex<double>(0.0, 0.0);
    y1 = std::complex<double>(0.0, 0.0);
    y2 = std::complex<double>(0.0, 0.0);
}

// ============================================================
// 构造函数
// ============================================================

RADAR_DUC_Block::RADAR_DUC_Block(const std::string& name)
    : Block(name)
{
}

// ============================================================
// Setup
// ============================================================

bool RADAR_DUC_Block::Setup()
{
    Block::Setup();
    while(!m_outputQueue.empty()) m_outputQueue.pop();
    while(!m_inputQueue.empty()) m_inputQueue.pop();
    m_outputCount = 0;
    return true;
}

// ============================================================
// Run — 运行模式分发
// ============================================================

bool RADAR_DUC_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

// ============================================================
// Initialize
// ============================================================

bool RADAR_DUC_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_duc = std::make_unique<RADAR_DUC>();
    SetDefaultParameters();

    try { IF_Freq = std::stod(getParameter("IF_Freq").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'IF_Freq', using default value."); }
    try { IF_SamplingRate = std::stod(getParameter("IF_SamplingRate").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'IF_SamplingRate', using default value."); }
    try { BandWidth = std::stod(getParameter("BandWidth").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'BandWidth', using default value."); }
    try { In_CenterFreq = std::stod(getParameter("In_CenterFreq").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'In_CenterFreq', using default value."); }
    try { BB_UpSamplingRatio = std::stoi(getParameter("BB_UpSamplingRatio").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'BB_UpSamplingRatio', using default value."); }
    try { RC_ExcessBW = std::stod(getParameter("RC_ExcessBW").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'RC_ExcessBW', using default value."); }
    try { PhaseImbalance = std::stod(getParameter("PhaseImbalance").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'PhaseImbalance', using default value."); }
    try { DAC_NBits = std::stoi(getParameter("DAC_NBits").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'DAC_NBits', using default value."); }

    SetParameters();

    if(!ModelSetup()) return false;

    AddInputPort("BB_Signal", m_duc->BB_Signal, 1, DataType::CIRCULAR_BUFFER_DCOMPLEX);
    AddOutputPort("IF_Signal", m_duc->IF_Signal, static_cast<size_t>(outRate_), DataType::ENVELOPE_SIGNAL);

    return true;
}

// ============================================================
// SetDefaultParameters
// ============================================================

void RADAR_DUC_Block::SetDefaultParameters()
{
    IF_Freq = 25000000.0;
    IF_SamplingRate = 100000000.0;
    BandWidth = 5000000.0;
    In_CenterFreq = 0.0;
    BB_UpSamplingRatio = 20;
    RC_ExcessBW = 0.22;
    PhaseImbalance = 0.0;
    DAC_NBits = 8;
}

// ============================================================
// SetParameters — 将 Block 参数同步到算法实例（仅用于端口注册）
// ============================================================

void RADAR_DUC_Block::SetParameters()
{
    if(!m_duc) return;
    m_duc->IF_Freq = IF_Freq;
    m_duc->IF_SamplingRate = IF_SamplingRate;
    m_duc->BandWidth = BandWidth;
    m_duc->In_CenterFreq = In_CenterFreq;
    m_duc->BB_UpSamplingRatio = BB_UpSamplingRatio;
    m_duc->RC_ExcessBW = RC_ExcessBW;
    m_duc->PhaseImbalance = PhaseImbalance;
    m_duc->DAC_NBits = DAC_NBits;
}

// ============================================================
// ModelSetup — Block 自行初始化 FIR/BPF，不调用 m_duc->Setup()
// ============================================================

bool RADAR_DUC_Block::ModelSetup()
{
    upRate_ = (BB_UpSamplingRatio > 0) ? BB_UpSamplingRatio : 1;
    outRate_ = upRate_;
    if(outRate_ < 1) outRate_ = 1;

    outputSampleRateHz_ = (IF_SamplingRate > 0.0) ? IF_SamplingRate : 0.0;
    outputTimeStepSec_ = (outputSampleRateHz_ > 0.0) ? (1.0 / outputSampleRateHz_) : 0.0;

    m_totalSamplesProcessed = 0;
    m_outputCount = 0;

    // 构建 FIR 滤波器
    buildRaisedCosineFir_();

    // 重置 FIR 状态
    ducFirState_.clear();
    if(!ducFir_.empty()) {
        ducFirState_.resize(ducFir_.size(), std::complex<double>(0.0, 0.0));
    }

    // 配置 IF BPF
    configureIfBpf_();

    // 重置 BPF 状态
    ifBpfSec1_.reset();
    ifBpfSec2_.reset();

    return true;
}

// ============================================================
// UpdateCharacterizationFrequency
// ============================================================

void RADAR_DUC_Block::UpdateCharacterizationFrequency()
{
    if(!m_duc) return;
    auto* outputBuffer = GetOutputPort(GetOutputPortName(0));
    if(outputBuffer) {
        outputBuffer->setCharacterizationFrequency(IF_Freq);
    }
}

// ============================================================
// DataStreamRun — 固定步长 DUC 处理
// ============================================================

bool RADAR_DUC_Block::DataStreamRun()
{
    std::string inputPortName = GetInputPortName(0);
    std::string outputPortName = GetOutputPortName(0);

    // 读取 1 个基带复数输入样本
    auto inputData = ReadInputData<std::complex<double>>(inputPortName);
    if(inputData.empty()) return true;

    const std::complex<double> input = inputData[0];
    const int totalOut = (outRate_ > 0) ? outRate_ : 1;

    std::vector<EnvelopeSignal> outputData;
    outputData.reserve(static_cast<size_t>(totalOut));

    for(int outIdx = 0; outIdx < totalOut; ++outIdx) {
        const double absSampleIndex =
            static_cast<double>(m_totalSamplesProcessed) * static_cast<double>(totalOut) +
            static_cast<double>(outIdx);

        const double timeNow =
            (outputTimeStepSec_ > 0.0) ?
            absSampleIndex * outputTimeStepSec_ :
            absSampleIndex;

        // 1. 上采样：第一个位置放输入，其余补零
        const std::complex<double> upsampled = (outIdx == 0) ? input : std::complex<double>(0.0, 0.0);

        // 2. Raised-cosine 插值滤波
        std::complex<double> x = runDucInterpolationFir_(upsampled);

        // 3. 输入中心频率搬移
        x = applyInputCenterFrequency_(x, timeNow);

        // 4. I/Q → IF envelope 转换（含相位不平衡校正）
        std::complex<double> y = applyDUCToIFEnvelope_(x, timeNow);

        // 5. DAC 量化效应补偿
        if(DAC_NBits >= 2 && DAC_NBits < 64) {
            const double ph = 2.0 * M_PI * IF_Freq * timeNow;
            const double realIfBefore = y.real() * std::cos(ph) - y.imag() * std::sin(ph);
            const double realIfAfter = applyDAC_(realIfBefore);
            const double err = realIfAfter - realIfBefore;
            y += std::complex<double>(err * std::cos(ph), -err * std::sin(ph));
        }

        // 6. IF BPF 滤波
        y = runIfBpf_(y);

        // 7. FcChange 镜像补偿
        y = applyFcChangeImage_(y, timeNow);

        // 8. 复数共轭约定
        y = applyFinalComplexConvention_(y, timeNow);

        outputData.push_back(EnvelopeSignal(y));
    }

    // 传播载波频率
    UpdateCharacterizationFrequency();

    // 写出输出数据
    WriteOutputData(outputPortName, outputData);

    m_totalSamplesProcessed += 1;
    return true;
}

// ============================================================
// TimeDrivenRun — 变步长逐点累积 DUC 模式
// ============================================================

bool RADAR_DUC_Block::TimeDrivenRun()
{
    std::string inputPortName = GetInputPortName(0);
    std::string outputPortName = GetOutputPortName(0);

    // 读取输入样本并累积到队列
    auto inputData = ReadInputData<std::complex<double>>(inputPortName);
    for(const auto& sample : inputData) {
        m_inputQueue.push(sample);
    }

    // 如果有输入数据可处理
    if(!m_inputQueue.empty()) {
        std::complex<double> inputSample = m_inputQueue.front();
        m_inputQueue.pop();

        const int totalOut = (outRate_ > 0) ? outRate_ : 1;

        for(int outIdx = 0; outIdx < totalOut; ++outIdx) {
            const double absSampleIndex =
                static_cast<double>(m_totalSamplesProcessed) * static_cast<double>(totalOut) +
                static_cast<double>(outIdx);

            const double timeNow =
                (outputTimeStepSec_ > 0.0) ?
                absSampleIndex * outputTimeStepSec_ :
                absSampleIndex;

            const std::complex<double> upsampled = (outIdx == 0) ? inputSample : std::complex<double>(0.0, 0.0);

            std::complex<double> x = runDucInterpolationFir_(upsampled);
            x = applyInputCenterFrequency_(x, timeNow);
            std::complex<double> y = applyDUCToIFEnvelope_(x, timeNow);

            if(DAC_NBits >= 2 && DAC_NBits < 64) {
                const double ph = 2.0 * M_PI * IF_Freq * timeNow;
                const double realIfBefore = y.real() * std::cos(ph) - y.imag() * std::sin(ph);
                const double realIfAfter = applyDAC_(realIfBefore);
                const double err = realIfAfter - realIfBefore;
                y += std::complex<double>(err * std::cos(ph), -err * std::sin(ph));
            }

            y = runIfBpf_(y);
            y = applyFcChangeImage_(y, timeNow);
            y = applyFinalComplexConvention_(y, timeNow);

            m_outputQueue.push(EnvelopeSignal(y));
        }

        m_totalSamplesProcessed += 1;
    }

    // 从输出队列取一个样本写出
    if(!m_outputQueue.empty()) {
        EnvelopeSignal outValue = m_outputQueue.front();
        m_outputQueue.pop();
        m_outputCount++;

        WriteOutputData(outputPortName, std::vector<EnvelopeSignal>{outValue});

        UpdateCharacterizationFrequency();
    }

    return true;
}

// ============================================================
// DUC 信号处理辅助函数（从 RADAR_DUC.cpp 复制）
// ============================================================

void RADAR_DUC_Block::buildRaisedCosineFir_()
{
    ducFir_.clear();

    const int sps = (upRate_ > 0) ? upRate_ : 1;
    const int spanSymbols = 30;
    const int nTaps = spanSymbols * sps + 1;
    const int mid = nTaps / 2;

    ducFir_.resize(nTaps, 0.0);

    const double alpha = clamp(RC_ExcessBW, 0.0, 1.0);
    double sum = 0.0;

    for(int n = 0; n < nTaps; ++n) {
        const double t = static_cast<double>(n - mid) / static_cast<double>(sps);
        ducFir_[n] = raisedCosineImpulse_(t, alpha);
        sum += ducFir_[n];
    }

    if(std::fabs(sum) > 1e-30) {
        const double scale = static_cast<double>(sps) / sum;
        for(size_t i = 0; i < ducFir_.size(); ++i) {
            ducFir_[i] *= scale;
        }
    }
}

std::complex<double> RADAR_DUC_Block::runDucInterpolationFir_(const std::complex<double>& x)
{
    if(ducFir_.empty()) return x;

    if(ducFirState_.size() != ducFir_.size()) {
        ducFirState_.clear();
        ducFirState_.resize(ducFir_.size(), std::complex<double>(0.0, 0.0));
    }

    ducFirState_.push_front(x);
    while(ducFirState_.size() > ducFir_.size()) {
        ducFirState_.pop_back();
    }

    std::complex<double> y(0.0, 0.0);
    for(size_t i = 0; i < ducFir_.size(); ++i) {
        y += ducFir_[i] * ducFirState_[i];
    }

    return y;
}

void RADAR_DUC_Block::configureIfBpf_()
{
    ifBpfEnabled_ = false;

    if(outputSampleRateHz_ <= 0.0 || BandWidth <= 0.0) {
        return;
    }

    double fc = 0.5 * BandWidth;
    if(fc <= 0.0) {
        return;
    }
    if(fc > 0.45 * outputSampleRateHz_) {
        fc = 0.45 * outputSampleRateHz_;
    }

    const double q = 0.7071067811865476;
    const double w0 = 2.0 * M_PI * fc / outputSampleRateHz_;
    const double alpha = std::sin(w0) / (2.0 * q);
    const double cosw = std::cos(w0);
    const double a0 = 1.0 + alpha;

    ifBpfSec1_.b0 = (1.0 - cosw) * 0.5 / a0;
    ifBpfSec1_.b1 = (1.0 - cosw) / a0;
    ifBpfSec1_.b2 = (1.0 - cosw) * 0.5 / a0;
    ifBpfSec1_.a1 = (-2.0 * cosw) / a0;
    ifBpfSec1_.a2 = (1.0 - alpha) / a0;

    ifBpfSec2_ = ifBpfSec1_;

    ifBpfEnabled_ = true;
}

std::complex<double> RADAR_DUC_Block::runBiquad_(const std::complex<double>& x, BiquadState& s)
{
    const std::complex<double> y =
        s.b0 * x +
        s.b1 * s.x1 +
        s.b2 * s.x2 -
        s.a1 * s.y1 -
        s.a2 * s.y2;

    s.x2 = s.x1;
    s.x1 = x;

    s.y2 = s.y1;
    s.y1 = y;

    return y;
}

std::complex<double> RADAR_DUC_Block::runIfBpf_(const std::complex<double>& x)
{
    if(!ifBpfEnabled_) return x;

    std::complex<double> y = x;
    y = runBiquad_(y, ifBpfSec1_);
    y = runBiquad_(y, ifBpfSec2_);
    return y;
}

std::complex<double> RADAR_DUC_Block::applyInputCenterFrequency_(const std::complex<double>& x, double timeNow) const
{
    if(std::fabs(In_CenterFreq) < 1e-15) return x;

    const double ph = 2.0 * M_PI * In_CenterFreq * timeNow;
    return x * std::complex<double>(std::cos(ph), std::sin(ph));
}

std::complex<double> RADAR_DUC_Block::applyDUCToIFEnvelope_(const std::complex<double>& x, double timeNow) const
{
    (void)timeNow;

    const double phi = deg2rad(PhaseImbalance);
    const double i = x.real();
    const double q = x.imag();

    return std::complex<double>(i - q * std::sin(phi), q * std::cos(phi));
}

std::complex<double> RADAR_DUC_Block::applyFcChangeImage_(const std::complex<double>& idealEnvelope, double timeNow) const
{
    const double imageFactorSteady = 0.70;
    const double imagePhaseDeg = -90.0;
    const double imageTimeAdvanceSec = 2.0e-6;

    const double startupBegin = 95.0e-6;
    const double startupEnd = 135.0e-6;

    double ramp = 1.0;
    if(timeNow <= startupBegin) {
        ramp = 0.0;
    }
    else if(timeNow < startupEnd) {
        const double u = (timeNow - startupBegin) / (startupEnd - startupBegin);
        ramp = u * u * (3.0 - 2.0 * u);
    }

    const double startupImageRatio = 0.55;
    const double imageFactor = imageFactorSteady *
        (startupImageRatio + (1.0 - startupImageRatio) * ramp);

    const double tImage = timeNow + imageTimeAdvanceSec;
    const double ph = 4.0 * M_PI * IF_Freq * tImage + deg2rad(imagePhaseDeg);
    const std::complex<double> rot(std::cos(ph), std::sin(ph));

    return idealEnvelope + imageFactor * std::conj(idealEnvelope) * rot;
}

std::complex<double> RADAR_DUC_Block::applyFinalComplexConvention_(const std::complex<double>& x, double timeNow) const
{
    (void)timeNow;
    return std::conj(x);
}

double RADAR_DUC_Block::applyDAC_(double x) const
{
    if(DAC_NBits < 2 || DAC_NBits >= 64) return x;

    const double fullScale = 1.0;
    const int bits = (DAC_NBits > 30) ? 30 : DAC_NBits;
    const int levels = 1 << bits;
    const double step = (2.0 * fullScale) / static_cast<double>(levels - 1);

    const double clipped = clamp(x, -fullScale, fullScale);
    const double q = std::floor((clipped + fullScale) / step + 0.5) * step - fullScale;

    return clamp(q, -fullScale, fullScale);
}

// ============================================================
// 数学工具函数
// ============================================================

double RADAR_DUC_Block::raisedCosineImpulse_(double t, double alpha)
{
    if(std::fabs(t) < 1e-12) return 1.0;

    if(alpha <= 1e-12) return sinc_(t);

    const double denom = 1.0 - std::pow(2.0 * alpha * t, 2.0);
    if(std::fabs(denom) < 1e-10) {
        return (M_PI / 4.0) * sinc_(1.0 / (2.0 * alpha));
    }

    return sinc_(t) * std::cos(M_PI * alpha * t) / denom;
}

double RADAR_DUC_Block::sinc_(double x)
{
    if(std::fabs(x) < 1e-12) return 1.0;
    const double pix = M_PI * x;
    return std::sin(pix) / pix;
}

double RADAR_DUC_Block::deg2rad(double x)
{
    return x * M_PI / 180.0;
}

double RADAR_DUC_Block::clamp(double x, double lo, double hi)
{
    return std::max(lo, std::min(hi, x));
}
