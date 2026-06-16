#include "RADAR_DDC_Block.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

RADAR_DDC_Block::RADAR_DDC_Block(const std::string &name)
    :Block(name)
{

}

bool RADAR_DDC_Block::Setup()
{
    Block::Setup();
    while(!m_outputQueue.empty()) m_outputQueue.pop();
    return true;
}

bool RADAR_DDC_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool RADAR_DDC_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_ddc = std::make_unique<RADAR_DDC>();
    SetDefaultParameters();

    try { IF_Freq = std::stod(getParameter("IF_Freq").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'IF_Freq', using default value."); }
    try { IF_SamplingRate = std::stod(getParameter("IF_SamplingRate").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'IF_SamplingRate', using default value."); }
    try { ADC_NBits = std::stoi(getParameter("ADC_NBits").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'ADC_NBits', using default value."); }
    try { PhaseImbalance = std::stod(getParameter("PhaseImbalance").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'PhaseImbalance', using default value."); }
    try { BB_DownSamplingRatio = std::stoi(getParameter("BB_DownSamplingRatio").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'BB_DownSamplingRatio', using default value."); }
    try { RC_ExcessBW = std::stod(getParameter("RC_ExcessBW").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'RC_ExcessBW', using default value."); }
    try { Out_CenterFreq = std::stod(getParameter("Out_CenterFreq").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Out_CenterFreq', using default value."); }

    SetParameters();

    if(!ModelSetup()) return false;

    AddInputPort("IF_Signal", m_ddc->IF_Signal, static_cast<size_t>(decim_), DataType::ENVELOPE_SIGNAL);
    AddOutputPort("BB_Signal", m_ddc->BB_Signal, 1, DataType::CIRCULAR_BUFFER_DCOMPLEX);
    return true;
}

void RADAR_DDC_Block::SetDefaultParameters()
{
    IF_Freq = 25000000.0;
    IF_SamplingRate = 100000000.0;
    ADC_NBits = 8;
    PhaseImbalance = 0.0;
    BB_DownSamplingRatio = 20;
    RC_ExcessBW = 0.22;
    Out_CenterFreq = 0.0;
}

void RADAR_DDC_Block::UpdateCharacterizationFrequency()
{
    if(!m_ddc) return;
    auto* inputReader = GetInputPort("IF_Signal");
    auto* outputBuffer = GetOutputPort("BB_Signal");
    if(inputReader && outputBuffer) {
        double fc = inputReader->getCharacterizationFrequency();
        outputBuffer->setCharacterizationFrequency(fc);
    }
}

bool RADAR_DDC_Block::ModelSetup()
{
    decim_ = (BB_DownSamplingRatio > 0) ? BB_DownSamplingRatio : 1;
    if(decim_ < 1) decim_ = 1;

    inputSampleRateHz_ = (IF_SamplingRate > 0.0) ? IF_SamplingRate : 0.0;
    inputTimeStepSec_ = (inputSampleRateHz_ > 0.0) ? (1.0 / inputSampleRateHz_) : 0.0;

    m_totalSamplesProcessed = 0;

    buildQuadSampleFir_();
    quadFirState_.clear();
    if(!quadFir_.empty()) {
        quadFirState_.resize(quadFir_.size(), std::complex<double>(0.0, 0.0));
    }

    return true;
}

void RADAR_DDC_Block::SetParameters()
{
    if(!m_ddc) return;
    m_ddc->IF_Freq = IF_Freq;
    m_ddc->IF_SamplingRate = IF_SamplingRate;
    m_ddc->ADC_NBits = ADC_NBits;
    m_ddc->PhaseImbalance = PhaseImbalance;
    m_ddc->BB_DownSamplingRatio = BB_DownSamplingRatio;
    m_ddc->RC_ExcessBW = RC_ExcessBW;
    m_ddc->Out_CenterFreq = Out_CenterFreq;
}

// ============================================================
// DataStreamRun
// ============================================================

bool RADAR_DDC_Block::DataStreamRun()
{
    auto inputData = ReadInputData<EnvelopeSignal>("IF_Signal");
    if(inputData.empty()) return true;

    UpdateCharacterizationFrequency();

    auto* inputReader = GetInputPort("IF_Signal");
    const double inputFcHz = inputReader ? inputReader->getCharacterizationFrequency() : 0.0;

    const int totalIn = decim_;

    std::complex<double> yFiltered(0.0, 0.0);
    double timeLast = 0.0;

    for(int i = 0; i < totalIn; ++i) {
        double timeNow = 0.0;
        if(inputTimeStepSec_ > 0.0) {
            timeNow = (static_cast<double>(m_totalSamplesProcessed) + static_cast<double>(i)) * inputTimeStepSec_;
        }
        timeLast = timeNow;

        const EnvelopeSignal& xinEnv = inputData[static_cast<size_t>(i)];

        // 1. envelope -> 实 IF
        double realIf = envelopeToRealIF_(xinEnv, inputFcHz, timeNow);

        // 2. ADC 量化
        realIf = applyADC_(realIf);

        // 3. 正交采样
        std::complex<double> z = quadSampleOneIFPoint_(realIf, timeNow);

        // 4. RC 低通/抽取滤波
        yFiltered = runQuadSampleFir_(z);
    }

    // 5. 输出
    std::complex<double> y = yFiltered;

    // 6. Out_CenterFreq 频率搬移
    y = applyOutCenterFreq_(y, timeLast);

    WriteOutputData("BB_Signal", std::vector<std::complex<double>>{y});

    m_totalSamplesProcessed += totalIn;
    return true;
}

// ============================================================
// TimeDrivenRun
// ============================================================

bool RADAR_DDC_Block::TimeDrivenRun()
{
    auto inputData = ReadInputData<EnvelopeSignal>("IF_Signal");
    if(inputData.empty()) return true;

    UpdateCharacterizationFrequency();

    for(const auto& val : inputData) m_inputBuffer.push_back(val);

    if(m_inputBuffer.size() >= static_cast<size_t>(decim_)) {
        auto* inputReader = GetInputPort("IF_Signal");
        const double inputFcHz = inputReader ? inputReader->getCharacterizationFrequency() : 0.0;

        const int totalIn = decim_;

        std::complex<double> yFiltered(0.0, 0.0);
        double timeLast = 0.0;

        for(int i = 0; i < totalIn; ++i) {
            double timeNow = 0.0;
            if(inputTimeStepSec_ > 0.0) {
                timeNow = (static_cast<double>(m_totalSamplesProcessed) + static_cast<double>(i)) * inputTimeStepSec_;
            }
            timeLast = timeNow;

            const EnvelopeSignal& xinEnv = m_inputBuffer[static_cast<size_t>(i)];

            double realIf = envelopeToRealIF_(xinEnv, inputFcHz, timeNow);
            realIf = applyADC_(realIf);
            std::complex<double> z = quadSampleOneIFPoint_(realIf, timeNow);
            yFiltered = runQuadSampleFir_(z);
        }

        std::complex<double> y = yFiltered;
        y = applyOutCenterFreq_(y, timeLast);

        m_outputQueue.push(y);
        m_totalSamplesProcessed += totalIn;
        m_inputBuffer.clear();
    }

    if(!m_outputQueue.empty()) {
        auto outputValue = m_outputQueue.front();
        m_outputQueue.pop();
        m_outputCount++;
        WriteOutputData("BB_Signal", std::vector<std::complex<double>>{outputValue});
        m_lastOutput = outputValue;
        qDebug() << "[RADAR_DDC_Block] 分发输出:" << m_outputCount
                 << " value:" << outputValue.real() << "," << outputValue.imag();
    }
    return true;
}

// ============================================================
// DDC 信号处理辅助函数（从 RADAR_DDC 复制）
// ============================================================

void RADAR_DDC_Block::buildQuadSampleFir_()
{
    quadFir_.clear();

    const int sps = (decim_ > 0) ? decim_ : 1;
    const int spanSymbols = 22;
    const int nTaps = spanSymbols * sps + 1;
    const int mid = nTaps / 2;

    quadFir_.resize(nTaps, 0.0);

    const double alpha = clamp(RC_ExcessBW, 0.0, 1.0);
    double sum = 0.0;

    for(int n = 0; n < nTaps; ++n) {
        const double t = static_cast<double>(n - mid) / static_cast<double>(sps);
        quadFir_[n] = raisedCosineImpulse_(t, alpha);
        sum += quadFir_[n];
    }

    if(std::fabs(sum) > 1e-30) {
        const double scale = 1.0 / sum;
        for(size_t i = 0; i < quadFir_.size(); ++i) {
            quadFir_[i] *= scale;
        }
    }
}

std::complex<double> RADAR_DDC_Block::runQuadSampleFir_(const std::complex<double>& x)
{
    if(quadFir_.empty()) return x;

    if(quadFirState_.size() != quadFir_.size()) {
        quadFirState_.clear();
        quadFirState_.resize(quadFir_.size(), std::complex<double>(0.0, 0.0));
    }

    quadFirState_.push_front(x);
    while(quadFirState_.size() > quadFir_.size()) {
        quadFirState_.pop_back();
    }

    std::complex<double> y(0.0, 0.0);
    for(size_t i = 0; i < quadFir_.size(); ++i) {
        y += quadFir_[i] * quadFirState_[i];
    }

    return y;
}

std::complex<double> RADAR_DDC_Block::envelopeToComplex_(
    const EnvelopeSignal& x, double fcHz) const
{
    if(fcHz > 0.0) return x.complex();
    return std::complex<double>(x.real(), 0.0);
}

double RADAR_DDC_Block::envelopeToRealIF_(
    const EnvelopeSignal& x, double inputFcHz, double timeNow) const
{
    const double fc = (inputFcHz > 0.0) ? inputFcHz : IF_Freq;
    const auto env = envelopeToComplex_(x, fc);

    if(fc <= 0.0) return env.real();

    const double ph = 2.0 * M_PI * fc * timeNow;
    const std::complex<double> carrier(std::cos(ph), std::sin(ph));

    return (env * carrier).real();
}

double RADAR_DDC_Block::applyADC_(double x) const
{
    if(ADC_NBits < 2 || ADC_NBits >= 32) return x;

    const double clipped = clamp(x, -1.0, 1.0);
    const int levels = 1 << ADC_NBits;
    const double step = 2.0 / static_cast<double>(levels - 1);
    const double q = std::floor((clipped + 1.0) / step + 0.5) * step - 1.0;

    return clamp(q, -1.0, 1.0);
}

std::complex<double> RADAR_DDC_Block::quadSampleOneIFPoint_(
    double realIf, double timeNow) const
{
    const double ph = 2.0 * M_PI * IF_Freq * timeNow;
    const double phi = deg2rad(PhaseImbalance);

    const double iRaw = 2.0 * realIf * std::cos(ph);
    const double qRaw = -2.0 * realIf * std::sin(ph + phi);

    return std::complex<double>(iRaw, qRaw);
}

std::complex<double> RADAR_DDC_Block::applyOutCenterFreq_(
    const std::complex<double>& x, double timeNow) const
{
    if(std::fabs(Out_CenterFreq) < 1e-15) return x;

    const double ph = 2.0 * M_PI * Out_CenterFreq * timeNow;
    return x * std::complex<double>(std::cos(ph), std::sin(ph));
}

// ============================================================
// 数学工具函数
// ============================================================

double RADAR_DDC_Block::raisedCosineImpulse_(double t, double alpha)
{
    if(std::fabs(t) < 1e-12) return 1.0;

    if(alpha > 1e-12) {
        const double singular = 1.0 / (2.0 * alpha);
        if(std::fabs(std::fabs(t) - singular) < 1e-10) {
            return 0.5 * sinc_(singular);
        }
    }

    const double pix = M_PI * t;
    const double num = std::sin(pix) / pix;
    const double den = 1.0 - 4.0 * alpha * alpha * t * t;

    if(std::fabs(den) < 1e-12) return 0.0;

    return num * std::cos(M_PI * alpha * t) / den;
}

double RADAR_DDC_Block::sinc_(double x)
{
    if(std::fabs(x) < 1e-12) return 1.0;
    const double pix = M_PI * x;
    return std::sin(pix) / pix;
}

double RADAR_DDC_Block::deg2rad(double x)
{
    return x * M_PI / 180.0;
}

double RADAR_DDC_Block::clamp(double x, double lo, double hi)
{
    if(x < lo) return lo;
    if(x > hi) return hi;
    return x;
}
