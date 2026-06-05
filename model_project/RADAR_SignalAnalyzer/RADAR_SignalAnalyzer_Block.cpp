#include "RADAR_SignalAnalyzer_Block.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <string>

// ============================================================================
// 匿名命名空间 — 纯静态工具函数
// ============================================================================

namespace {

std::string TrimCopy(const std::string& value)
{
    std::string s = value;
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
    return s;
}

std::string ToLowerCopy(const std::string& value)
{
    std::string s = value;
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return s;
}

// ---------- FFT ----------

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

int factorial(int n)
{
    int result = 1;
    for (int i = 1; i <= n; ++i) result *= i;
    return result;
}

double I0(int n, double x)
{
    double I0_x = 1.0;
    for (int i = 1; i <= n; ++i) {
        double term = std::pow(x / 2.0, i) / static_cast<double>(factorial(i));
        I0_x += term * term;
    }
    return I0_x;
}

SystemVueModelBuilder::Matrix<std::complex<double>> autoCorr(
    SystemVueModelBuilder::Matrix<std::complex<double>>& A, int LenA)
{
    SystemVueModelBuilder::Matrix<std::complex<double>> result(1, 2 * LenA - 1);
    result.Zero();
    for (int i = 0; i < LenA; ++i) {
        for (int j = 0; j < LenA; ++j) {
            result(i + j) += A(i) * std::conj(A(LenA - j - 1));
        }
    }
    return result;
}

} // anonymous namespace

// ============================================================================
// 构造函数
// ============================================================================

RADAR_SignalAnalyzer_Block::RADAR_SignalAnalyzer_Block(const std::string& name)
    : Block(name)
    , m_AnalyzerType(RADAR_SignalAnalyzer::FFT)
    , m_WindowType(RADAR_SignalAnalyzer::Rectangle)
    , m_WindowParameter(1.0)
    , m_CorrType(RADAR_SignalAnalyzer::Normal)
    , m_NormalizedType(RADAR_SignalAnalyzer::Normalized)
    , m_FFTShiftType(RADAR_SignalAnalyzer::NonShift)
    , m_SampleNum(1024)
    , m_FFTSize(1024)
    , m_SampleRate(10e6)
{
}

// ============================================================================
// SetDefaultParameters
// ============================================================================

void RADAR_SignalAnalyzer_Block::SetDefaultParameters()
{
    m_AnalyzerType    = RADAR_SignalAnalyzer::FFT;
    m_WindowType      = RADAR_SignalAnalyzer::Rectangle;
    m_WindowParameter = 1.0;
    m_CorrType        = RADAR_SignalAnalyzer::Normal;
    m_NormalizedType  = RADAR_SignalAnalyzer::Normalized;
    m_FFTShiftType    = RADAR_SignalAnalyzer::NonShift;
    m_SampleNum       = 1024;
    m_FFTSize         = 1024;
    m_SampleRate      = 10e6;
}

// ============================================================================
// SetParameters — 将解析后的参数写入算法对象
// ============================================================================

void RADAR_SignalAnalyzer_Block::SetParameters()
{
    if (!m_algo) return;
    m_algo->AnalyzerType    = m_AnalyzerType;
    m_algo->WindowType      = m_WindowType;
    m_algo->WindowParameter = m_WindowParameter;
    m_algo->CorrType        = m_CorrType;
    m_algo->NormalizedType  = m_NormalizedType;
    m_algo->FFTShiftType    = m_FFTShiftType;
    m_algo->SampleNum       = m_SampleNum;
    m_algo->FFTSize         = m_FFTSize;
    m_algo->SampleRate      = m_SampleRate;
}

// ============================================================================
// ConvertStringTo* 系列
// ============================================================================

RADAR_SignalAnalyzer::SelectedAnalyzerType
RADAR_SignalAnalyzer_Block::ConvertStringToAnalyzerType(const std::string& value)
{
    const std::string v = ToLowerCopy(TrimCopy(value));
    if (v == "fft"  || v == "0") return RADAR_SignalAnalyzer::FFT;
    if (v == "ifft" || v == "1") return RADAR_SignalAnalyzer::IFFT;
    if (v == "acf"  || v == "2") return RADAR_SignalAnalyzer::ACF;
    return RADAR_SignalAnalyzer::FFT;
}

RADAR_SignalAnalyzer::SelectedWindowType
RADAR_SignalAnalyzer_Block::ConvertStringToWindowType(const std::string& value)
{
    const std::string v = ToLowerCopy(TrimCopy(value));
    if (v == "rectangle"     || v == "0") return RADAR_SignalAnalyzer::Rectangle;
    if (v == "bartlett"      || v == "1") return RADAR_SignalAnalyzer::Bartlett;
    if (v == "hanning"       || v == "2") return RADAR_SignalAnalyzer::Hanning;
    if (v == "hamming"       || v == "3") return RADAR_SignalAnalyzer::Hamming;
    if (v == "blackman"      || v == "4") return RADAR_SignalAnalyzer::Blackman;
    if (v == "steepblackman" || v == "5") return RADAR_SignalAnalyzer::SteepBlackman;
    if (v == "kaiser"        || v == "6") return RADAR_SignalAnalyzer::Kaiser;
    return RADAR_SignalAnalyzer::Rectangle;
}

RADAR_SignalAnalyzer::SelectedCorrType
RADAR_SignalAnalyzer_Block::ConvertStringToCorrType(const std::string& value)
{
    const std::string v = ToLowerCopy(TrimCopy(value));
    if (v == "normal"   || v == "0") return RADAR_SignalAnalyzer::Normal;
    if (v == "biased"   || v == "1") return RADAR_SignalAnalyzer::Biased;
    if (v == "unbiased" || v == "2") return RADAR_SignalAnalyzer::UnBiased;
    return RADAR_SignalAnalyzer::Normal;
}

RADAR_SignalAnalyzer::SelectedNormalizedType
RADAR_SignalAnalyzer_Block::ConvertStringToNormalizedType(const std::string& value)
{
    const std::string v = ToLowerCopy(TrimCopy(value));
    if (v == "normalized"    || v == "0") return RADAR_SignalAnalyzer::Normalized;
    if (v == "nonnormalized" || v == "1") return RADAR_SignalAnalyzer::NonNormalized;
    return RADAR_SignalAnalyzer::Normalized;
}

RADAR_SignalAnalyzer::SelectedFFTShiftType
RADAR_SignalAnalyzer_Block::ConvertStringToFFTShiftType(const std::string& value)
{
    const std::string v = ToLowerCopy(TrimCopy(value));
    if (v == "shifted"  || v == "0") return RADAR_SignalAnalyzer::Shifted;
    if (v == "nonshift" || v == "1") return RADAR_SignalAnalyzer::NonShift;
    return RADAR_SignalAnalyzer::NonShift;
}

// ============================================================================
// Setup
// ============================================================================

bool RADAR_SignalAnalyzer_Block::Setup()
{
    Block::Setup();

    if (m_SampleNum <= 0) {
        LOG_ERROR("SampleNum must be > 0");
    }
    if (m_FFTSize < m_SampleNum) {
        LOG_ERROR("FFTSize must be >= SampleNum");
    }
    if ((m_FFTSize & (m_FFTSize - 1)) != 0) {
        LOG_ERROR("Only 2^N FFTSize is supported now. For FFTSize != 2^N, performance may be insufficient.");
    }
    if (m_SampleRate <= 0.0) {
        LOG_ERROR("SampleRate must be > 0");
    }

    m_inputBuffer.clear();
    while (!m_outputQueue.empty()) m_outputQueue.pop();

    return true;
}

// ============================================================================
// Run — 双模式分发
// ============================================================================

bool RADAR_SignalAnalyzer_Block::Run()
{
    if (IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

// ============================================================================
// ProcessFrame — 帧处理（内联移植自 RADAR_SignalAnalyzer::Run）
// ============================================================================

void RADAR_SignalAnalyzer_Block::ProcessFrame(
    const std::vector<std::complex<double>>& inputData,
    std::vector<double>& outputData)
{
    const double PI = std::acos(-1.0);
    const int fftSize   = m_FFTSize;
    const int sampleNum = m_SampleNum;
    const int windowN   = fftSize - 1;

    outputData.resize(static_cast<size_t>(sampleNum));

    // ============== 窗函数 ==============

    SystemVueModelBuilder::Matrix<std::complex<double>> WindowSequence(1, fftSize);

    switch (m_WindowType) {
    case RADAR_SignalAnalyzer::Rectangle:
        for (int i = 0; i < fftSize; ++i) WindowSequence(i) = 1.0;
        break;

    case RADAR_SignalAnalyzer::Bartlett:
        for (int i = 0; i < fftSize; ++i) {
            if (i < fftSize / 2)
                WindowSequence(i) = 2.0 * static_cast<double>(i) / static_cast<double>(windowN);
            else if (i < fftSize)
                WindowSequence(i) = 2.0 - 2.0 * static_cast<double>(i) / static_cast<double>(windowN);
            else
                WindowSequence(i) = 0.0;
        }
        break;

    case RADAR_SignalAnalyzer::Hanning:
        for (int i = 0; i < fftSize; ++i) {
            if (i < fftSize)
                WindowSequence(i) = 0.5 * (1.0 - std::cos(2.0 * PI * static_cast<double>(i) / static_cast<double>(windowN)));
            else
                WindowSequence(i) = 0.0;
        }
        break;

    case RADAR_SignalAnalyzer::Hamming:
        for (int i = 0; i < fftSize; ++i) {
            if (i < fftSize)
                WindowSequence(i) = 0.54 - 0.46 * std::cos(2.0 * PI * static_cast<double>(i) / static_cast<double>(windowN));
            else
                WindowSequence(i) = 0.0;
        }
        break;

    case RADAR_SignalAnalyzer::Blackman:
        for (int i = 0; i < fftSize; ++i) {
            if (i < fftSize / 2)
                WindowSequence(i) = 0.42 - 0.5 * std::cos(2.0 * PI * static_cast<double>(i) / static_cast<double>(windowN))
                                      + 0.08 * std::cos(4.0 * PI * static_cast<double>(i) / static_cast<double>(windowN));
            else if (i < fftSize)
                WindowSequence(i) = 0.42 - 0.5 * std::cos(2.0 * PI * static_cast<double>(fftSize - i) / static_cast<double>(windowN))
                                      + 0.08 * std::cos(4.0 * PI * static_cast<double>(fftSize - i) / static_cast<double>(windowN));
            else
                WindowSequence(i) = 0.0;
        }
        break;

    case RADAR_SignalAnalyzer::SteepBlackman:
        for (int i = 0; i < fftSize; ++i) {
            if (i < fftSize / 2) {
                const double w = static_cast<double>(i);
                WindowSequence(i) = 0.35875
                    - 0.48829 * std::cos(2.0 * PI * w / static_cast<double>(windowN))
                    + 0.14128 * std::cos(4.0 * PI * w / static_cast<double>(windowN))
                    - 0.01168 * std::cos(6.0 * PI * w / static_cast<double>(windowN));
            } else if (i < fftSize) {
                const double w = static_cast<double>(fftSize - i);
                WindowSequence(i) = 0.35875
                    - 0.48829 * std::cos(2.0 * PI * w / static_cast<double>(windowN))
                    + 0.14128 * std::cos(4.0 * PI * w / static_cast<double>(windowN))
                    - 0.01168 * std::cos(6.0 * PI * w / static_cast<double>(windowN));
            } else {
                WindowSequence(i) = 0.0;
            }
        }
        break;

    case RADAR_SignalAnalyzer::Kaiser:
        for (int i = 0; i < fftSize; ++i) {
            if (i < fftSize) {
                const double r = 2.0 * static_cast<double>(i) / static_cast<double>(windowN) - 1.0;
                WindowSequence(i) = I0(20, m_WindowParameter * std::sqrt(1.0 - r * r))
                                  / I0(20, m_WindowParameter);
            } else {
                WindowSequence(i) = 0.0;
            }
        }
        break;
    }

    // ============== 后处理 ==============

    switch (m_AnalyzerType) {

    // ---------- FFT ----------
    case RADAR_SignalAnalyzer::FFT: {
        SystemVueModelBuilder::Matrix<std::complex<double>> FullSequence(1, fftSize);
        for (int i = 0; i < fftSize; ++i) {
            if (i < sampleNum)
                FullSequence(i) = inputData[static_cast<size_t>(i)];
            else
                FullSequence(i) = 0.0;
        }

        fft(FullSequence, fftSize, 1);

        if (m_NormalizedType == RADAR_SignalAnalyzer::NonNormalized)
            FullSequence *= static_cast<double>(fftSize);

        for (int i = 0; i < fftSize; ++i)
            FullSequence(i) *= WindowSequence(i);

        if (m_FFTShiftType == RADAR_SignalAnalyzer::NonShift) {
            for (int i = 0; i < sampleNum; ++i)
                outputData[static_cast<size_t>(i)] = std::abs(FullSequence(i));
        } else {
            for (int i = 0; i < sampleNum; ++i) {
                int n = i - fftSize / 2;
                outputData[static_cast<size_t>(i)] = std::abs(FullSequence(n >= 0 ? n : n + fftSize));
            }
        }
        break;
    }

    // ---------- IFFT ----------
    case RADAR_SignalAnalyzer::IFFT: {
        SystemVueModelBuilder::Matrix<std::complex<double>> FullSequence(1, fftSize);
        for (int i = 0; i < fftSize; ++i) {
            if (i < sampleNum)
                FullSequence(i) = inputData[static_cast<size_t>(i)];
            else
                FullSequence(i) = 0.0;
        }

        SystemVueModelBuilder::Matrix<std::complex<double>> ShiftSequence(1, fftSize);
        if (m_FFTShiftType == RADAR_SignalAnalyzer::NonShift) {
            for (int i = 0; i < fftSize; ++i)
                ShiftSequence(i) = FullSequence(i);
        } else {
            for (int i = 0; i < fftSize; ++i) {
                int n = i + fftSize / 2;
                ShiftSequence(i) = FullSequence(n < fftSize ? n : n - fftSize);
            }
        }

        for (int i = 0; i < fftSize; ++i)
            ShiftSequence(i) *= WindowSequence(i);

        fft(ShiftSequence, fftSize, -1);

        if (m_NormalizedType == RADAR_SignalAnalyzer::NonNormalized)
            ShiftSequence *= static_cast<double>(fftSize);

        for (int i = 0; i < sampleNum; ++i) {
            int idx = fftSize - i;
            outputData[static_cast<size_t>(i)] = std::abs(ShiftSequence(idx < fftSize ? idx : 0));
        }
        break;
    }

    // ---------- ACF ----------
    case RADAR_SignalAnalyzer::ACF: {
        SystemVueModelBuilder::Matrix<std::complex<double>> FullSequence(1, sampleNum);
        for (int i = 0; i < sampleNum; ++i)
            FullSequence(i) = inputData[static_cast<size_t>(i)];

        SystemVueModelBuilder::Matrix<std::complex<double>> CorrSequence = autoCorr(FullSequence, sampleNum);

        for (int i = 0; i < sampleNum; ++i)
            outputData[static_cast<size_t>(i)] = std::abs(CorrSequence(i + sampleNum - 1));
        break;
    }
    }
}

// ============================================================================
// DataStreamRun — 数据流模式
// ============================================================================

bool RADAR_SignalAnalyzer_Block::DataStreamRun()
{
    auto inputData = ReadInputData<std::complex<double>>(GetInputPortName(0));
    if (inputData.empty()) return true;

    std::vector<double> outputData;
    ProcessFrame(inputData, outputData);

    WriteOutputData(GetOutputPortName(0), outputData);
    return true;
}

// ============================================================================
// TimeDrivenRun — 变步长模式
// ============================================================================

bool RADAR_SignalAnalyzer_Block::TimeDrivenRun()
{
    const int threshold = (m_SampleNum > 0) ? m_SampleNum : 1024;

    // ① 累积输入
    {
        auto inputData = ReadInputData<std::complex<double>>(GetInputPortName(0));
        if (inputData.empty()) return true;
        m_inputBuffer.push_back(inputData[0]);
    }

    // ② 判断阈值 + 处理
    if (static_cast<int>(m_inputBuffer.size()) >= threshold) {
        std::vector<double> outputData;
        ProcessFrame(m_inputBuffer, outputData);

        for (const auto& val : outputData)
            m_outputQueue.push(val);

        m_inputBuffer.erase(m_inputBuffer.begin(),
                            m_inputBuffer.begin() + static_cast<size_t>(threshold));
    }

    // ③ 出队写入
    if (!m_outputQueue.empty()) {
        std::vector<double> outVec;
        outVec.push_back(m_outputQueue.front());
        WriteOutputData(GetOutputPortName(0), outVec);
        m_outputQueue.pop();
    }

    return true;
}

// ============================================================================
// Initialize
// ============================================================================

bool RADAR_SignalAnalyzer_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_algo = std::make_unique<RADAR_SignalAnalyzer>();

    SetDefaultParameters();

    try { m_AnalyzerType    = ConvertStringToAnalyzerType(getParameter("AnalyzerType").Value);    } catch (...) {}
    try { m_WindowType      = ConvertStringToWindowType(getParameter("WindowType").Value);        } catch (...) {}
    try { m_WindowParameter = std::stod(getParameter("WindowParameter").Value);                   } catch (...) {}
    try { m_CorrType        = ConvertStringToCorrType(getParameter("CorrType").Value);            } catch (...) {}
    try { m_NormalizedType  = ConvertStringToNormalizedType(getParameter("NormalizedType").Value);} catch (...) {}
    try { m_FFTShiftType    = ConvertStringToFFTShiftType(getParameter("FFTShiftType").Value);    } catch (...) {}
    try { m_SampleNum       = std::stoi(getParameter("SampleNum").Value);                         } catch (...) {}
    try { m_FFTSize         = std::stoi(getParameter("FFTSize").Value);                           } catch (...) {}
    try { m_SampleRate      = std::stod(getParameter("SampleRate").Value);                        } catch (...) {}

    SetParameters();

    const int rate = (m_SampleNum > 0) ? m_SampleNum : 1024;

    AddInputPort("input",  m_algo->input,  rate, Block::DataType::CIRCULAR_BUFFER_DCOMPLEX);
    AddOutputPort("output", m_algo->output, rate, Block::DataType::CIRCULAR_BUFFER_DOUBLE);

    return true;
}
