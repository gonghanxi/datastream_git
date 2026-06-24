#include "RADAR_QuadSample_Block.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================================================================
// 构造函数
// ============================================================================

RADAR_QuadSample_Block::RADAR_QuadSample_Block(const std::string& name)
    : Block(name)
    , BB_DownSamplingRatio(20)
    , IF_Freq(25000000.0)
    , IF_SamplingRate(100000000.0)
    , Out_CenterFreq(0.0)
    , PhaseImbalance(0.0)
    , RC_ExcessBW(0.22)
    , decim_(20)
    , inputSampleRateHz_(100000000.0)
    , outputSampleRateHz_(5000000.0)
    , inputTimeStepSec_(1.0 / 100000000.0)
    , m_count(0)
{
}

// ============================================================================
// Setup
// ============================================================================

bool RADAR_QuadSample_Block::Setup()
{
    Block::Setup();
    m_count = 0;
    // 清空时间驱动模式的缓冲区
    m_inputBuffer.clear();
    while(!m_outputQueue.empty()) {
        m_outputQueue.pop();
    }
    return true;
}

// ============================================================================
// Run — 运行模式分发
// ============================================================================

bool RADAR_QuadSample_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

// ============================================================================
// Initialize
// ============================================================================

bool RADAR_QuadSample_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_qs = std::make_unique<RADAR_QuadSample>();

    SetDefaultParameters();

    // 解析参数
    try { BB_DownSamplingRatio = std::stoi(getParameter("BB_DownSamplingRatio").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'BB_DownSamplingRatio', using default value."); }
    try { IF_Freq = std::stod(getParameter("IF_Freq").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'IF_Freq', using default value."); }
    try { IF_SamplingRate = std::stod(getParameter("IF_SamplingRate").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'IF_SamplingRate', using default value."); }
    try { Out_CenterFreq = std::stod(getParameter("Out_CenterFreq").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Out_CenterFreq', using default value."); }
    try { PhaseImbalance = std::stod(getParameter("PhaseImbalance").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'PhaseImbalance', using default value."); }
    try { RC_ExcessBW = std::stod(getParameter("RC_ExcessBW").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'RC_ExcessBW', using default value."); }

    SetParameters();

    if(!ModelSetup()) return false;

    // 注册端口（使用 m_qs 的端口对象进行注册）
    // IF_Signal: 输入 readSize = decim_（每次读取 decim_ 个 real 样本）
    AddInputPort("IF_Signal", m_qs->IF_Signal, static_cast<size_t>(decim_), DataType::CIRCULAR_BUFFER_DOUBLE);

    // BB_Signal: 输出 writeSize = 1（每次输出 1 个 complex 样本）
    AddOutputPort("BB_Signal", m_qs->BB_Signal, 1, DataType::CIRCULAR_BUFFER_DCOMPLEX);

    // 设置抽取因子
    SetDecimationFactor(decim_);

    return true;
}

// ============================================================================
// SetDefaultParameters
// ============================================================================

void RADAR_QuadSample_Block::SetDefaultParameters()
{
    BB_DownSamplingRatio = 20;
    IF_Freq = 25000000.0;
    IF_SamplingRate = 100000000.0;
    Out_CenterFreq = 0.0;
    PhaseImbalance = 0.0;
    RC_ExcessBW = 0.22;
}

// ============================================================================
// SetParameters — 将 Block 参数同步到算法实例（仅用于端口注册）
// ============================================================================

void RADAR_QuadSample_Block::SetParameters()
{
    if(!m_qs) return;
    m_qs->BB_DownSamplingRatio = BB_DownSamplingRatio;
    m_qs->IF_Freq = IF_Freq;
    m_qs->IF_SamplingRate = IF_SamplingRate;
    m_qs->Out_CenterFreq = Out_CenterFreq;
    m_qs->PhaseImbalance = PhaseImbalance;
    m_qs->RC_ExcessBW = RC_ExcessBW;
}

// ============================================================================
// ModelSetup — 初始化内部状态
// ============================================================================

bool RADAR_QuadSample_Block::ModelSetup()
{
    decim_ = (BB_DownSamplingRatio > 0) ? BB_DownSamplingRatio : 1;
    if(decim_ < 1) decim_ = 1;

    inputSampleRateHz_ = (IF_SamplingRate > 0.0) ? IF_SamplingRate : 0.0;
    inputTimeStepSec_ = (inputSampleRateHz_ > 0.0) ? (1.0 / inputSampleRateHz_) : 0.0;

    outputSampleRateHz_ = (inputSampleRateHz_ > 0.0) ?
        (inputSampleRateHz_ / static_cast<double>(decim_)) : 0.0;

    // 构建 FIR 滤波器
    buildQuadSampleFir_();

    // 初始化 FIR 状态
    quadFirState_.clear();
    if(!quadFir_.empty()) {
        quadFirState_.resize(quadFir_.size(), std::complex<double>(0.0, 0.0));
    }

    m_count = 0;

    return true;
}

// ============================================================================
// DataStreamRun — 数据流模式
// ============================================================================
//
// 每次 firing 读取 decim_ 个 IF real 样本，输出 1 个 BB complex 样本
// 速率比 = decim_ / 1 = decim_

bool RADAR_QuadSample_Block::DataStreamRun()
{
    std::string inputPortName = GetInputPortName(0);
    std::string outputPortName = GetOutputPortName(0);

    // 读取 decim_ 个 IF real 样本
    auto inputData = ReadInputData<double>(inputPortName);
    if(inputData.empty()) return true;

    const int totalIn = (decim_ > 0) ? decim_ : 1;

    // 正交采样处理
    std::complex<double> yFiltered(0.0, 0.0);
    double timeLast = 0.0;

    for(int i = 0; i < totalIn && i < static_cast<int>(inputData.size()); ++i) {
        double timeNow = 0.0;
        if(inputTimeStepSec_ > 0.0) {
            timeNow = (static_cast<double>(m_count) * static_cast<double>(totalIn) +
                       static_cast<double>(i)) * inputTimeStepSec_;
        }
        timeLast = timeNow;

        const double realIf = inputData[static_cast<size_t>(i)];

        // 1. 正交采样
        const std::complex<double> z = quadSampleOneIFPoint_(realIf, timeNow);

        // 2. FIR 低通滤波
        yFiltered = runQuadSampleFir_(z);
    }

    // 3. 降采样输出（每 decim_ 个输入产生 1 个输出）
    std::complex<double> y = yFiltered;

    // 4. 输出中心频率转换
    y = applyOutCenterFreq_(y, timeLast);

    // 写出输出
    WriteOutputData(outputPortName, std::vector<std::complex<double>>{y});

    m_count++;

    return true;
}

// ============================================================================
// TimeDrivenRun — 变步长模式
// ============================================================================
//
// 时间流模式：输入累积到缓冲区，输出推入队列，逐点写出

bool RADAR_QuadSample_Block::TimeDrivenRun()
{
    std::string inputPortName = GetInputPortName(0);
    std::string outputPortName = GetOutputPortName(0);

    // —— 步骤 1：读取输入并累积到缓冲区 ——
    auto inputData = ReadInputData<double>(inputPortName);
    for(const auto& sample : inputData) {
        m_inputBuffer.push_back(sample);
    }

    // —— 步骤 2：每累积 decim_ 个样本，执行一次处理 ——
    const int totalIn = (decim_ > 0) ? decim_ : 1;

    while(static_cast<int>(m_inputBuffer.size()) >= totalIn) {
        std::complex<double> yFiltered(0.0, 0.0);
        double timeLast = 0.0;

        for(int i = 0; i < totalIn; ++i) {
            double timeNow = 0.0;
            if(inputTimeStepSec_ > 0.0) {
                timeNow = (static_cast<double>(m_count) * static_cast<double>(totalIn) +
                           static_cast<double>(i)) * inputTimeStepSec_;
            }
            timeLast = timeNow;

            const double realIf = m_inputBuffer.front();
            m_inputBuffer.pop_front();

            // 1. 正交采样
            const std::complex<double> z = quadSampleOneIFPoint_(realIf, timeNow);

            // 2. FIR 低通滤波
            yFiltered = runQuadSampleFir_(z);
        }

        // 3. 降采样输出
        std::complex<double> y = yFiltered;

        // 4. 输出中心频率转换
        y = applyOutCenterFreq_(y, timeLast);

        m_outputQueue.push(y);
        m_count++;
    }

    // —— 步骤 3：从输出队列取一个数据输出 ——
    if(!m_outputQueue.empty()) {
        std::complex<double> output = m_outputQueue.front();
        m_outputQueue.pop();
        WriteOutputData(outputPortName, std::vector<std::complex<double>>{output});
    }

    return true;
}

// ============================================================================
// FIR 滤波器构建（raised-cosine 低通滤波器）
// ============================================================================

void RADAR_QuadSample_Block::buildQuadSampleFir_()
{
    quadFir_.clear();

    const int sps = (decim_ > 0) ? decim_ : 1;
    const int spanSymbols = 22;
    const int nTaps = spanSymbols * sps + 1;
    const int mid = nTaps / 2;

    quadFir_.resize(static_cast<size_t>(nTaps), 0.0);

    const double alpha = clamp(RC_ExcessBW, 0.0, 1.0);
    double sum = 0.0;

    for(int n = 0; n < nTaps; ++n) {
        const double t = static_cast<double>(n - mid) / static_cast<double>(sps);
        quadFir_[static_cast<size_t>(n)] = raisedCosineImpulse_(t, alpha);
        sum += quadFir_[static_cast<size_t>(n)];
    }

    // 归一化
    if(std::fabs(sum) > 1e-30) {
        const double scale = 1.0 / sum;
        for(size_t i = 0; i < quadFir_.size(); ++i) {
            quadFir_[i] *= scale;
        }
    }
}

std::complex<double> RADAR_QuadSample_Block::runQuadSampleFir_(const std::complex<double>& x)
{
    if(quadFir_.empty()) {
        return x;
    }

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

// ============================================================================
// 正交采样
// ============================================================================

std::complex<double> RADAR_QuadSample_Block::quadSampleOneIFPoint_(double realIf, double timeNow) const
{
    const double ph = 2.0 * M_PI * IF_Freq * timeNow;
    const double phi = deg2rad(PhaseImbalance);

    const double iRaw = realIf * std::cos(ph);
    const double qRaw = -realIf * std::sin(ph + phi);

    return std::complex<double>(iRaw, qRaw);
}

// ============================================================================
// 输出中心频率转换
// ============================================================================

std::complex<double> RADAR_QuadSample_Block::applyOutCenterFreq_(const std::complex<double>& x, double timeNow) const
{
    if(std::fabs(Out_CenterFreq) < 1e-15) {
        return x;
    }

    const double ph = 2.0 * M_PI * Out_CenterFreq * timeNow;
    return x * std::complex<double>(std::cos(ph), std::sin(ph));
}

// ============================================================================
// 数学辅助函数
// ============================================================================

double RADAR_QuadSample_Block::raisedCosineImpulse_(double t, double alpha)
{
    if(std::fabs(t) < 1e-12) {
        return 1.0;
    }

    if(alpha > 1e-12) {
        const double singular = 1.0 / (2.0 * alpha);
        if(std::fabs(std::fabs(t) - singular) < 1e-10) {
            return 0.5 * sinc_(singular);
        }
    }

    const double pix = M_PI * t;
    const double num = std::sin(pix) / pix;
    const double den = 1.0 - 4.0 * alpha * alpha * t * t;

    if(std::fabs(den) < 1e-12) {
        return 0.0;
    }

    return num * std::cos(M_PI * alpha * t) / den;
}

double RADAR_QuadSample_Block::sinc_(double x)
{
    if(std::fabs(x) < 1e-12) {
        return 1.0;
    }

    const double pix = M_PI * x;
    return std::sin(pix) / pix;
}

double RADAR_QuadSample_Block::deg2rad(double x)
{
    return x * M_PI / 180.0;
}

double RADAR_QuadSample_Block::clamp(double x, double lo, double hi)
{
    if(x < lo) return lo;
    if(x > hi) return hi;
    return x;
}
