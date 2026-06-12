#include "RADAR_LFMRef_Block.h"

#include <cmath>
#include <string>

// ============================================================================
// 匿名命名空间 — 纯静态工具函数
// ============================================================================

namespace {

void fft(SystemVueModelBuilder::Matrix<std::complex<double>>& a, int n, int invert)
{
    const double PI = std::acos(-1.0);
    if (n == 1) return;

    int half = n / 2;
    SystemVueModelBuilder::Matrix<std::complex<double>> even(1, half), odd(1, half);
    for (int i = 0; i < half; ++i) {
        even(i) = a(i * 2);
        odd(i)  = a(i * 2 + 1);
    }

    fft(even, half, invert);
    fft(odd,  half, invert);

    double angle = 2.0 * PI / static_cast<double>(n) * (invert ? -1.0 : 1.0);
    std::complex<double> w(1.0, 0.0);
    std::complex<double> wn(std::cos(angle), std::sin(angle));

    for (int i = 0; i < half; ++i) {
        a(i)        = even(i) + w * odd(i);
        a(i + half) = even(i) - w * odd(i);
        if (invert) {
            a(i)        /= 2.0;
            a(i + half) /= 2.0;
        }
        w *= wn;
    }
}

} // anonymous namespace

// ============================================================================
// 构造函数
// ============================================================================

RADAR_LFMRef_Block::RADAR_LFMRef_Block(const std::string& name)
    : Block(name)
    , m_Pulsewidth(1e-5)
    , m_Bandwidth(5e6)
    , m_FM_Offset(0.0)
    , m_SampleRate(10e6)
    , m_FFTSize(1024)
{
}

// ============================================================================
// SetDefaultParameters
// ============================================================================

void RADAR_LFMRef_Block::SetDefaultParameters()
{
    m_Pulsewidth = 1e-5;
    m_Bandwidth  = 5e6;
    m_FM_Offset  = 0.0;
    m_SampleRate = 10e6;
    m_FFTSize    = 1024;
}

// ============================================================================
// SetParameters — 将解析后的参数写入算法对象
// ============================================================================

void RADAR_LFMRef_Block::SetParameters()
{
    if (!m_algo) return;
    m_algo->Pulsewidth = m_Pulsewidth;
    m_algo->Bandwidth  = m_Bandwidth;
    m_algo->FM_Offset  = m_FM_Offset;
    m_algo->SampleRate = m_SampleRate;
    m_algo->FFTSize    = m_FFTSize;
}

// ============================================================================
// Setup
// ============================================================================

bool RADAR_LFMRef_Block::Setup()
{
    Block::Setup();

    if (m_Pulsewidth <= 0.0) {
        LOG_ERROR("Pulsewidth must be > 0");
    }
    if (m_Bandwidth <= 0.0) {
        LOG_ERROR("Bandwidth must be > 0");
    }
    if (m_SampleRate <= 0.0) {
        LOG_ERROR("SampleRate must be > 0");
    }
    if (m_FFTSize <= 0) {
        LOG_ERROR("FFTSize must be > 0");
    }
    if ((m_FFTSize & (m_FFTSize - 1)) != 0) {
        LOG_ERROR("Only 2^N FFTSize is supported now. For FFTSize != 2^N, performance may be insufficient.");
    }

    while (!m_outputQueue.empty()) m_outputQueue.pop();

    SetParameters();
    return true;
}

// ============================================================================
// Run — 双模式分发
// ============================================================================

bool RADAR_LFMRef_Block::Run()
{
    if (IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

// ============================================================================
// GenerateFrame — 帧生成（内联移植自 RADAR_LFMRef::Run）
// ============================================================================

void RADAR_LFMRef_Block::GenerateFrame(std::vector<std::complex<double>>& outputData)
{
    const double PI = std::acos(-1.0);
    const std::complex<double> imag_I(0.0, 1.0);
    const int fftSize = m_FFTSize;
    const int sampleCount = static_cast<int>(m_Pulsewidth * m_SampleRate);
    const double t0 = m_Pulsewidth / 2.0;

    outputData.resize(static_cast<size_t>(fftSize));

    // LFM信号序列
    SystemVueModelBuilder::Matrix<std::complex<double>> LFMSequence(1, fftSize);
    for (int i = 0; i < fftSize; ++i) {
        if (i < sampleCount) {
            double t = static_cast<double>(i) / m_SampleRate;
            LFMSequence(i) = std::exp(imag_I * PI * m_Bandwidth * std::pow(t - t0, 2.0) / m_Pulsewidth)
                           * std::exp(imag_I * 2.0 * PI * m_FM_Offset * t);
        } else {
            LFMSequence(i) = 0.0;
        }
    }

    // 共轭翻转
    SystemVueModelBuilder::Matrix<std::complex<double>> FFTSequence(1, fftSize);
    for (int i = 0; i < fftSize; ++i) {
        FFTSequence(i) = std::conj(LFMSequence(fftSize - i - 1));
    }

    // FFT
    fft(FFTSequence, fftSize, 1);

    // 按 FFT 点数加权
    FFTSequence *= static_cast<double>(fftSize);

    // 输出
    for (int i = 0; i < fftSize; ++i) {
        outputData[static_cast<size_t>(i)] = FFTSequence(i);
    }
}

// ============================================================================
// DataStreamRun — 数据流模式
// ============================================================================

bool RADAR_LFMRef_Block::DataStreamRun()
{
    std::vector<std::complex<double>> outputData;
    GenerateFrame(outputData);
    WriteOutputData(GetOutputPortName(0), outputData);
    return true;
}

// ============================================================================
// TimeDrivenRun — 变步长模式
// ============================================================================

bool RADAR_LFMRef_Block::TimeDrivenRun()
{
    // 队列空时生成新帧
    if (m_outputQueue.empty()) {
        std::vector<std::complex<double>> outputData;
        GenerateFrame(outputData);
        for (const auto& val : outputData) {
            m_outputQueue.push(val);
        }
    }

    // 出队写入
    if (!m_outputQueue.empty()) {
        std::vector<std::complex<double>> outVec;
        outVec.push_back(m_outputQueue.front());
        WriteOutputData(GetOutputPortName(0), outVec);
        m_outputQueue.pop();
    }

    return true;
}

// ============================================================================
// Initialize
// ============================================================================

bool RADAR_LFMRef_Block::Initialize()
{
    SetBlockType(Block::BlockType::SOURCE);

    m_algo = std::make_unique<RADAR_LFMRef>();

    SetDefaultParameters();

    try { m_Pulsewidth = std::stod(getParameter("Pulsewidth").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Pulsewidth', using default value."); }
    try { m_Bandwidth  = std::stod(getParameter("Bandwidth").Value);  } catch (...) { LOG_WARN("Failed to parse parameter 'Bandwidth', using default value."); }
    try { m_FM_Offset  = std::stod(getParameter("FM_Offset").Value);  } catch (...) { LOG_WARN("Failed to parse parameter 'FM_Offset', using default value."); }
    try { m_SampleRate = std::stod(getParameter("SampleRate").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'SampleRate', using default value."); }
    try { m_FFTSize    = std::stoi(getParameter("FFTSize").Value);    } catch (...) { LOG_WARN("Failed to parse parameter 'FFTSize', using default value."); }

    SetParameters();

    const int rate = (m_FFTSize > 0) ? m_FFTSize : 1024;

    AddOutputPort("output", m_algo->output, rate, Block::DataType::CIRCULAR_BUFFER_DCOMPLEX);

    return true;
}
