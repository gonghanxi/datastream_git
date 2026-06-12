#include "RADAR_PulseCompression_M_Block.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <string>

namespace {

std::string TrimCopy(const std::string& value)
{
    std::string s = value;
    s.erase(s.begin(), std::find_if(s.begin(), s.end(),
        [](unsigned char ch) { return !std::isspace(ch); }));
    s.erase(std::find_if(s.rbegin(), s.rend(),
        [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
    return s;
}

std::string ToLowerCopy(const std::string& value)
{
    std::string s = value;
    std::transform(s.begin(), s.end(), s.begin(),
        [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return s;
}

} // anonymous namespace

// ============================================================================
// ConvertStringToWindowType
// ============================================================================

RADAR_PulseCompression_M::SelectedWindowType
RADAR_PulseCompression_M_Block::ConvertStringToWindowType(const std::string& value)
{
    const std::string v = ToLowerCopy(TrimCopy(value));
    if (v == "rectangle"     || v == "0") return RADAR_PulseCompression_M::Rectangle;
    if (v == "bartlett"      || v == "1") return RADAR_PulseCompression_M::Bartlett;
    if (v == "hanning"       || v == "2") return RADAR_PulseCompression_M::Hanning;
    if (v == "hamming"       || v == "3") return RADAR_PulseCompression_M::Hamming;
    if (v == "blackman"      || v == "4") return RADAR_PulseCompression_M::Blackman;
    if (v == "steepblackman" || v == "5") return RADAR_PulseCompression_M::SteepBlackman;
    if (v == "kaiser"        || v == "6") return RADAR_PulseCompression_M::Kaiser;
    return RADAR_PulseCompression_M::Rectangle;
}

// ============================================================================
// 构造函数
// ============================================================================

RADAR_PulseCompression_M_Block::RADAR_PulseCompression_M_Block(const std::string& name)
    : Block(name)
    , m_WindowType(RADAR_PulseCompression_M::Rectangle)
    , m_WindowParameter(1.0)
{
}

// ============================================================================
// SetDefaultParameters
// ============================================================================

void RADAR_PulseCompression_M_Block::SetDefaultParameters()
{
    m_WindowType      = RADAR_PulseCompression_M::Rectangle;
    m_WindowParameter = 1.0;
}

// ============================================================================
// SetParameters
// ============================================================================

void RADAR_PulseCompression_M_Block::SetParameters()
{
    if (!m_algo) return;
    m_algo->WindowType      = m_WindowType;
    m_algo->WindowParameter = m_WindowParameter;
}

// ============================================================================
// validateAndPrepare
// ============================================================================

bool RADAR_PulseCompression_M_Block::validateAndPrepare()
{
    // 矩阵版参数简单，WindowType 为枚举，WindowParameter 不做严格检查。
    return true;
}

// ============================================================================
// Setup
// ============================================================================

bool RADAR_PulseCompression_M_Block::Setup()
{
    Block::Setup();

    m_refBuffer.clear();
    m_sigBuffer.clear();
    while (!m_outputQueue.empty()) m_outputQueue.pop();

    return true;
}

// ============================================================================
// Run
// ============================================================================

bool RADAR_PulseCompression_M_Block::Run()
{
    if (IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

// ============================================================================
// Initialize
// ============================================================================

bool RADAR_PulseCompression_M_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_algo = std::make_unique<RADAR_PulseCompression_M>();

    SetDefaultParameters();

    try { m_WindowType      = ConvertStringToWindowType(getParameter("WindowType").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'WindowType', using default value."); }
    try { m_WindowParameter = std::stod(getParameter("WindowParameter").Value);            } catch (...) { LOG_WARN("Failed to parse parameter 'WindowParameter', using default value."); }

    SetParameters();

    AddInputPort("reference", m_algo->reference, 1, Block::DataType::MATRIX_DCOMPLEX);
    AddInputPort("signal",    m_algo->signal,    1, Block::DataType::MATRIX_DCOMPLEX);
    AddOutputPort("output",   m_algo->output,     1, Block::DataType::MATRIX_DCOMPLEX);

    return true;
}

// ============================================================================
// DataStreamRun — 数据流模式
// ============================================================================

bool RADAR_PulseCompression_M_Block::DataStreamRun()
{
    auto refData = ReadInputData<CxMatrix>(GetInputPortName(0));
    auto sigData = ReadInputData<CxMatrix>(GetInputPortName(1));

    if (refData.empty() || sigData.empty()) return true;

    const CxMatrix& refMat = refData[0];
    const CxMatrix& sigMat = sigData[0];

    CxMatrix outMat = processOne(refMat, sigMat);

    WriteOutputData(GetOutputPortName(0), std::vector<CxMatrix>{outMat});

    return true;
}

// ============================================================================
// TimeDrivenRun — 变步长模式（rate=1，reference/signal 各 1 个 Matrix）
// ============================================================================

bool RADAR_PulseCompression_M_Block::TimeDrivenRun()
{
    // ① 累积输入
    {
        auto refData = ReadInputData<CxMatrix>(GetInputPortName(0));
        for (auto& v : refData) m_refBuffer.push_back(std::move(v));

        auto sigData = ReadInputData<CxMatrix>(GetInputPortName(1));
        for (auto& v : sigData) m_sigBuffer.push_back(std::move(v));
    }

    // ② 当 reference 和 signal 都有数据时，处理一对
    if (!m_refBuffer.empty() && !m_sigBuffer.empty())
    {
        CxMatrix refMat = std::move(m_refBuffer.front()); m_refBuffer.pop_front();
        CxMatrix sigMat = std::move(m_sigBuffer.front()); m_sigBuffer.pop_front();

        CxMatrix outMat = processOne(refMat, sigMat);
        m_outputQueue.push(std::move(outMat));
    }

    // ③ 出队写入
    if (!m_outputQueue.empty())
    {
        CxMatrix v = std::move(m_outputQueue.front()); m_outputQueue.pop();
        WriteOutputData(GetOutputPortName(0), std::vector<CxMatrix>{v});
    }

    return true;
}

// ============================================================================
// processOne — 单次脉冲压缩
// ============================================================================

RADAR_PulseCompression_M_Block::CxMatrix
RADAR_PulseCompression_M_Block::processOne(const CxMatrix& refMat, const CxMatrix& sigMat)
{
    const int sigRows = static_cast<int>(sigMat.NumRows());
    const int sigCols = static_cast<int>(sigMat.NumColumns());

    if (sigRows <= 0 || sigCols <= 0)
        return CxMatrix();

    const int Samplenum = sigCols;
    const int FFTSize   = getReferenceFFTSize(refMat);

    if (Samplenum < 1 || FFTSize < Samplenum)
        return CxMatrix(sigRows, Samplenum);

    // 生成窗函数
    Matrix<Cx> WindowSequence(1, FFTSize);
    buildWindowSequence(WindowSequence, FFTSize);

    const int WindowLen = FFTSize;
    const int winShift  = WindowLen / 2;

    CxMatrix outMat(sigRows, Samplenum);

    for (int row = 0; row < sigRows; ++row)
    {
        Matrix<Cx> FullSequence(1, FFTSize);

        // signal 当前行补零到 FFTSize
        for (int i = 0; i < FFTSize; i++)
        {
            if (i < Samplenum)
                FullSequence(i) = sigMat(row, i);
            else
                FullSequence(i) = 0.0;
        }

        // FFT
        fft(FullSequence, FFTSize, 1);
        FullSequence *= FFTSize;

        // 乘 reference 频谱
        for (int i = 0; i < FFTSize; i++)
            FullSequence(i) *= getReferenceValue(refMat, row, i);

        // 频域加窗（循环移位）
        for (int i = 0; i < FFTSize; i++)
        {
            const int wi = (i + winShift < FFTSize) ? (i + winShift) : (i + winShift - FFTSize);
            FullSequence(i) *= WindowSequence(wi);
        }

        // IFFT
        fft(FullSequence, FFTSize, -1);

        // 取结果（FFTSize - i 索引）
        for (int i = 0; i < Samplenum; i++)
        {
            const int src = (FFTSize - i < FFTSize) ? (FFTSize - i) : 0;
            outMat(row, i) = FullSequence(src);
        }
    }

    return outMat;
}

// ============================================================================
// buildWindowSequence — 频域窗函数生成
// ============================================================================

void RADAR_PulseCompression_M_Block::buildWindowSequence(Matrix<Cx>& WindowSequence, int fftSize)
{
    const double PI = acos(-1.0);

    const int FFTSize   = std::max(1, fftSize);
    const int WindowLen = FFTSize;
    const int WindowN   = std::max(1, WindowLen - 1);

    WindowSequence.Resize(1, FFTSize);

    switch (m_WindowType)
    {
    case RADAR_PulseCompression_M::Rectangle:
    {
        for (int i = 0; i < FFTSize; i++)
            WindowSequence(i) = 1.0;
        break;
    }
    case RADAR_PulseCompression_M::Bartlett:
    {
        for (int i = 0; i < FFTSize; i++)
        {
            if (i < WindowLen / 2)
                WindowSequence(i) = 2.0 * i / WindowN;
            else if (i >= WindowLen / 2 && i < WindowLen)
                WindowSequence(i) = 2.0 - 2.0 * i / WindowN;
            else
                WindowSequence(i) = 0.0;
        }
        break;
    }
    case RADAR_PulseCompression_M::Hanning:
    {
        for (int i = 0; i < FFTSize; i++)
        {
            if (i < WindowLen)
                WindowSequence(i) = 0.5 * (1.0 - cos(2.0 * PI * i / WindowN));
            else
                WindowSequence(i) = 0.0;
        }
        break;
    }
    case RADAR_PulseCompression_M::Hamming:
    {
        for (int i = 0; i < FFTSize; i++)
        {
            if (i < WindowLen)
                WindowSequence(i) = 0.54 - 0.46 * cos(2.0 * PI * i / WindowN);
            else
                WindowSequence(i) = 0.0;
        }
        break;
    }
    case RADAR_PulseCompression_M::Blackman:
    {
        for (int i = 0; i < FFTSize; i++)
        {
            if (i < WindowLen / 2)
            {
                WindowSequence(i) = 0.42
                    - 0.5 * cos(2.0 * PI * i / WindowN)
                    + 0.08 * cos(4.0 * PI * i / WindowN);
            }
            else if (i >= WindowLen / 2 && i < WindowLen)
            {
                WindowSequence(i) = 0.42
                    - 0.5 * cos(2.0 * PI * (WindowLen - i) / WindowN)
                    + 0.08 * cos(4.0 * PI * (WindowLen - i) / WindowN);
            }
            else
            {
                WindowSequence(i) = 0.0;
            }
        }
        break;
    }
    case RADAR_PulseCompression_M::SteepBlackman:
    {
        for (int i = 0; i < FFTSize; i++)
        {
            if (i < WindowLen / 2)
            {
                WindowSequence(i) = 0.35875
                    - 0.48829 * cos(2.0 * PI * i / WindowN)
                    + 0.14128 * cos(4.0 * PI * i / WindowN)
                    - 0.01168 * cos(6.0 * PI * i / WindowN);
            }
            else if (i >= WindowLen / 2 && i < WindowLen)
            {
                WindowSequence(i) = 0.35875
                    - 0.48829 * cos(2.0 * PI * (WindowLen - i) / WindowN)
                    + 0.14128 * cos(4.0 * PI * (WindowLen - i) / WindowN)
                    - 0.01168 * cos(6.0 * PI * (WindowLen - i) / WindowN);
            }
            else
            {
                WindowSequence(i) = 0.0;
            }
        }
        break;
    }
    case RADAR_PulseCompression_M::Kaiser:
    {
        for (int i = 0; i < FFTSize; i++)
        {
            if (i < WindowLen)
            {
                const double t = 2.0 * i / WindowN - 1.0;
                const double v = std::max(0.0, 1.0 - t * t);
                WindowSequence(i) = I0(20, m_WindowParameter * sqrt(v)) / I0(20, m_WindowParameter);
            }
            else
            {
                WindowSequence(i) = 0.0;
            }
        }
        break;
    }
    default:
    {
        for (int i = 0; i < FFTSize; i++)
            WindowSequence(i) = 1.0;
        break;
    }
    }
}

// ============================================================================
// 递归 FFT
// ============================================================================

void RADAR_PulseCompression_M_Block::fft(Matrix<Cx>& a, int n, int invert)
{
    const double PI = acos(-1.0);

    if (n == 1) return;

    int half = n / 2;
    Matrix<Cx> even(1, half), odd(1, half);

    for (int i = 0; i < half; i++)
    {
        even(i) = a(i * 2);
        odd(i)  = a(i * 2 + 1);
    }

    fft(even, half, invert);
    fft(odd, half, invert);

    double angle = 2.0 * PI / n * (invert ? -1.0 : 1.0);
    Cx w(1.0, 0.0), wn(cos(angle), sin(angle));

    for (int i = 0; i < half; i++)
    {
        a(i)        = even(i) + w * odd(i);
        a(i + half) = even(i) - w * odd(i);

        if (invert)
        {
            a(i)        /= 2.0;
            a(i + half) /= 2.0;
        }

        w *= wn;
    }
}

// ============================================================================
// 阶乘（Kaiser 窗 I0 近似用）
// ============================================================================

int RADAR_PulseCompression_M_Block::factorial(int n)
{
    int result = 1;
    for (int i = 1; i <= n; ++i)
        result *= i;
    return result;
}

// ============================================================================
// 第一类零阶修正贝塞尔函数 I0 近似
// ============================================================================

double RADAR_PulseCompression_M_Block::I0(int n, double x)
{
    double I0_x = 1.0;
    for (int i = 1; i <= n; ++i)
    {
        I0_x += pow((pow(x / 2.0, i) / factorial(i)), 2.0);
    }
    return I0_x;
}

// ============================================================================
// 从 reference 矩阵尺寸推断 FFTSize
// ============================================================================

int RADAR_PulseCompression_M_Block::getReferenceFFTSize(const CxMatrix& ref) const
{
    const int rows = static_cast<int>(ref.NumRows());
    const int cols = static_cast<int>(ref.NumColumns());

    if (cols > 1) return cols;
    if (rows > 1) return rows;

    return cols;
}

// ============================================================================
// 读取 reference 频谱值
// ============================================================================

RADAR_PulseCompression_M_Block::Cx
RADAR_PulseCompression_M_Block::getReferenceValue(const CxMatrix& ref, int row, int k) const
{
    const int rows = static_cast<int>(ref.NumRows());
    const int cols = static_cast<int>(ref.NumColumns());

    if (rows <= 0 || cols <= 0) return Cx(0.0, 0.0);

    // reference 为行向量或多行频谱矩阵
    if (cols > 1)
    {
        int rr = 0;
        if (rows > 1)
        {
            rr = row;
            if (rr < 0) rr = 0;
            if (rr >= rows) rr = rows - 1;
        }

        if (k < 0 || k >= cols) return Cx(0.0, 0.0);
        return ref(rr, k);
    }

    // reference 为列向量
    if (k < 0 || k >= rows) return Cx(0.0, 0.0);
    return ref(k, 0);
}
