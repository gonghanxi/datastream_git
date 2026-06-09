#include "RADAR_MTD_M_Block.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <sstream>
#include <string>

// ============================================================================
// 匿名命名空间 — 纯静态工具函数
// ============================================================================

namespace {

// ---------- 字符串工具 ----------

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

// ---------- 数组参数解析 ----------

std::vector<double> ParseStringToDoubleVector(const std::string& value)
{
    std::vector<double> result;

    // 去除首尾空格
    std::string s = value;
    size_t start = s.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return result;
    size_t end = s.find_last_not_of(" \t\n\r");
    s = s.substr(start, end - start + 1);

    // 严格要求 [...] 格式
    if (s.empty() || s.front() != '[' || s.back() != ']') {
        return result;
    }

    // 去除外层括号
    std::string content = s.substr(1, s.length() - 2);

    // 去除括号内首尾空格
    start = content.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return result;  // 空数组
    end = content.find_last_not_of(" \t\n\r");
    content = content.substr(start, end - start + 1);

    // 按逗号分割
    std::stringstream ss(content);
    std::string item;
    while (std::getline(ss, item, ',')) {
        start = item.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) continue;
        end = item.find_last_not_of(" \t\n\r");
        item = item.substr(start, end - start + 1);

        if (!item.empty()) {
            try {
                result.push_back(std::stod(item));
            } catch (...) {
                return {};
            }
        }
    }

    return result;
}

// ---------- FFT / DFT ----------

constexpr double kPI = 3.14159265358979323846;

bool isPowerOfTwo(int n) { return n > 0 && ((n & (n - 1)) == 0); }

void backupFFT(std::vector<std::complex<double>>& x)
{
    const size_t N = x.size();
    if (N <= 1) return;

    std::vector<std::complex<double>> even(N / 2), odd(N / 2);
    for (size_t i = 0; i < N / 2; ++i) {
        even[i] = x[2 * i];
        odd[i]  = x[2 * i + 1];
    }

    backupFFT(even);
    backupFFT(odd);

    for (size_t k = 0; k < N / 2; ++k) {
        const double angle = -2.0 * kPI * static_cast<double>(k) / static_cast<double>(N);
        const std::complex<double> twiddle(std::cos(angle), std::sin(angle));
        const std::complex<double> t = twiddle * odd[k];
        x[k]         = even[k] + t;
        x[k + N / 2] = even[k] - t;
    }
}

void directDFT(std::vector<std::complex<double>>& x)
{
    const int N = static_cast<int>(x.size());
    if (N <= 1) return;

    std::vector<std::complex<double>> y(N, std::complex<double>(0.0, 0.0));
    for (int k = 0; k < N; ++k) {
        std::complex<double> sum(0.0, 0.0);
        for (int n = 0; n < N; ++n) {
            const double angle = -2.0 * kPI * static_cast<double>(k) * static_cast<double>(n) / static_cast<double>(N);
            sum += x[n] * std::complex<double>(std::cos(angle), std::sin(angle));
        }
        y[k] = sum;
    }
    x = y;
}

void internalFFT(std::vector<std::complex<double>>& x)
{
    const int N = static_cast<int>(x.size());
    if (N <= 1) return;
    if (isPowerOfTwo(N)) backupFFT(x);
    else                 directDFT(x);
}

// ---------- 贝塞尔 ----------

double modifiedBesselI0(double x)
{
    double sum = 1.0, term = 1.0;
    const double halfX2 = (x / 2.0) * (x / 2.0);
    for (int k = 1; k <= 30; ++k) {
        term *= halfX2 / static_cast<double>(k * k);
        sum  += term;
        if (std::abs(term) < 1e-15) break;
    }
    return sum;
}

// ---------- 窗函数生成 ----------

std::vector<double> generateBartlettWindow(int size)
{
    std::vector<double> w(size, 1.0);
    if (size <= 1) return w;
    const double M = static_cast<double>(size - 1);
    for (int n = 0; n < size; ++n)
        w[n] = 1.0 - std::abs((static_cast<double>(n) - M / 2.0) / (M / 2.0));
    return w;
}

std::vector<double> generateHanningWindow(int size)
{
    std::vector<double> w(size, 1.0);
    if (size <= 1) return w;
    const double M = static_cast<double>(size - 1);
    for (int n = 0; n < size; ++n)
        w[n] = 0.5 - 0.5 * std::cos(2.0 * kPI * static_cast<double>(n) / M);
    return w;
}

std::vector<double> generateHammingWindow(int size)
{
    std::vector<double> w(size, 1.0);
    if (size <= 1) return w;
    const double M = static_cast<double>(size - 1);
    for (int n = 0; n < size; ++n)
        w[n] = 0.54 - 0.46 * std::cos(2.0 * kPI * static_cast<double>(n) / M);
    return w;
}

std::vector<double> generateBlackmanWindow(int size)
{
    std::vector<double> w(size, 1.0);
    if (size <= 1) return w;
    const double M = static_cast<double>(size - 1);
    for (int n = 0; n < size; ++n) {
        const double a = 2.0 * kPI * static_cast<double>(n) / M;
        w[n] = 0.42 - 0.50 * std::cos(a) + 0.08 * std::cos(2.0 * a);
    }
    return w;
}

std::vector<double> generateSteepBlackmanWindow(int size)
{
    std::vector<double> w(size, 1.0);
    if (size <= 1) return w;
    const double M = static_cast<double>(size - 1);
    for (int n = 0; n < size; ++n) {
        const double a = 2.0 * kPI * static_cast<double>(n) / M;
        w[n] = 0.35875 - 0.48829 * std::cos(a) + 0.14128 * std::cos(2.0 * a) - 0.01168 * std::cos(3.0 * a);
    }
    return w;
}

std::vector<double> generateKaiserWindow(int size, double beta)
{
    std::vector<double> w(size, 1.0);
    if (size <= 1) return w;
    const double denom = modifiedBesselI0(beta);
    const double M = static_cast<double>(size - 1);
    if (denom == 0.0) return w;
    for (int n = 0; n < size; ++n) {
        const double ratio = (2.0 * static_cast<double>(n)) / M - 1.0;
        w[n] = modifiedBesselI0(beta * std::sqrt(std::max(0.0, 1.0 - ratio * ratio))) / denom;
    }
    return w;
}

} // anonymous namespace

// ============================================================================
// 构造函数
// ============================================================================

RADAR_MTD_M_Block::RADAR_MTD_M_Block(const std::string& name)
    : Block(name)
    , m_WindowType(RADAR_MTD_M::Rectangle)
    , m_NumOfPulse(8)
    , m_Freq_Weight(nullptr)
    , m_Freq_Weight_Size(0)
    , m_WindowParameters(nullptr)
    , m_WindowParameters_Size(0)
{
}

// ============================================================================
// SetDefaultParameters
// ============================================================================

void RADAR_MTD_M_Block::SetDefaultParameters()
{
    m_WindowType           = RADAR_MTD_M::Rectangle;
    m_NumOfPulse           = 8;
    m_Freq_Weight          = nullptr;
    m_Freq_Weight_Size     = 0;
    m_WindowParameters     = nullptr;
    m_WindowParameters_Size = 0;
    m_Freq_WeightData      = { 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0 };
    m_WindowParametersData = { 0.0 };
}

// ============================================================================
// SetParameters — 将解析后的参数写入算法对象
// ============================================================================

void RADAR_MTD_M_Block::SetParameters()
{
    // 指针指向存储 vector 的首地址
    m_Freq_Weight          = m_Freq_WeightData.empty()          ? nullptr : m_Freq_WeightData.data();
    m_Freq_Weight_Size     = static_cast<int>(m_Freq_WeightData.size());
    m_WindowParameters     = m_WindowParametersData.empty()    ? nullptr : m_WindowParametersData.data();
    m_WindowParameters_Size = static_cast<int>(m_WindowParametersData.size());

    if (!m_algo) return;
    m_algo->WindowType = m_WindowType;
    m_algo->NumOfPulse = m_NumOfPulse;
    m_algo->Freq_Weight = m_Freq_Weight;
    m_algo->Freq_Weight_Size = m_Freq_Weight_Size;
    m_algo->WindowParameters = m_WindowParameters;
    m_algo->WindowParameters_Size = m_WindowParameters_Size;
}

// ============================================================================
// ConvertStringToWindowType
// ============================================================================

RADAR_MTD_M::SelectedWindowType
RADAR_MTD_M_Block::ConvertStringToWindowType(const std::string& value)
{
    const std::string v = ToLowerCopy(TrimCopy(value));
    if (v == "rectangle"     || v == "0") return RADAR_MTD_M::Rectangle;
    if (v == "bartlett"      || v == "1") return RADAR_MTD_M::Bartlett;
    if (v == "hanning"       || v == "2") return RADAR_MTD_M::Hanning;
    if (v == "hamming"       || v == "3") return RADAR_MTD_M::Hamming;
    if (v == "blackman"      || v == "4") return RADAR_MTD_M::Blackman;
    if (v == "steepblackman" || v == "5") return RADAR_MTD_M::SteepBlackman;
    if (v == "kaiser"        || v == "6") return RADAR_MTD_M::Kaiser;
    return RADAR_MTD_M::Rectangle;
}

// ============================================================================
// Setup
// ============================================================================

bool RADAR_MTD_M_Block::Setup()
{
    Block::Setup();

    if (m_NumOfPulse <= 0) {
        m_NumOfPulse = 1;
    }

    GenerateWindow(m_NumOfPulse);

    m_inputBuffer.clear();
    while (!m_outputQueue.empty()) m_outputQueue.pop();

    return true;
}

// ============================================================================
// Run — 双模式分发
// ============================================================================

bool RADAR_MTD_M_Block::Run()
{
    return DataStreamRun();
}

// ============================================================================
// GenerateWindow — 窗函数生成（移植自 RADAR_MTD_M）
// ============================================================================

void RADAR_MTD_M_Block::GenerateWindow(int size)
{
    if (size <= 0) { m_window.clear(); return; }

    switch (m_WindowType) {
    case RADAR_MTD_M::Rectangle:    m_window.assign(size, 1.0);              break;
    case RADAR_MTD_M::Bartlett:     m_window = generateBartlettWindow(size);      break;
    case RADAR_MTD_M::Hanning:      m_window = generateHanningWindow(size);       break;
    case RADAR_MTD_M::Hamming:      m_window = generateHammingWindow(size);       break;
    case RADAR_MTD_M::Blackman:     m_window = generateBlackmanWindow(size);      break;
    case RADAR_MTD_M::SteepBlackman: m_window = generateSteepBlackmanWindow(size); break;
    case RADAR_MTD_M::Kaiser: {
        double beta = 0.0;
        if (m_WindowParameters != nullptr && m_WindowParameters_Size > 0)
            beta = m_WindowParameters[0];
        m_window = generateKaiserWindow(size, beta);
        break;
    }
    default: m_window.assign(size, 1.0); break;
    }
}

// ============================================================================
// ProcessOneSlowTimeVector — 单距离门慢时间处理（移植自 RADAR_MTD_M）
// ============================================================================

void RADAR_MTD_M_Block::ProcessOneSlowTimeVector(std::vector<std::complex<double>>& x)
{
    const int N = static_cast<int>(x.size());
    if (N <= 0) return;

    // 1. 三脉冲对消
    std::vector<std::complex<double>> y(N, std::complex<double>(0.0, 0.0));
    if (N == 1) {
        y[0] = x[0];
    } else if (N == 2) {
        y[0] = std::complex<double>(0.0, 0.0);
        y[1] = std::complex<double>(0.0, 0.0);
    } else {
        y[0] = std::complex<double>(0.0, 0.0);
        y[1] = std::complex<double>(0.0, 0.0);
        for (int p = 2; p < N; ++p)
            y[p] = x[p] - 2.0 * x[p - 1] + x[p - 2];
    }

    // 2. 时域加窗
    if (static_cast<int>(m_window.size()) != N)
        GenerateWindow(N);
    for (int p = 0; p < N; ++p)
        y[p] *= m_window[p];

    // 3. FFT / DFT
    internalFFT(y);

    // 4. 频域权重
    if (m_Freq_Weight != nullptr && m_Freq_Weight_Size > 0) {
        for (int k = 0; k < N && k < m_Freq_Weight_Size; ++k)
            y[k] *= m_Freq_Weight[k];
    }

    x = y;
}

// ============================================================================
// ProcessFrame — 帧处理（移植自 RADAR_MTD_M::Run）
// ============================================================================

void RADAR_MTD_M_Block::ProcessFrame(
    const SystemVueModelBuilder::DComplexMatrix& inMat,
    SystemVueModelBuilder::DComplexMatrix& outMat)
{
    const size_t nRows = inMat.NumRows();
    const size_t nCols = inMat.NumColumns();

    outMat.Resize(nRows, nCols);

    const int N = m_NumOfPulse;
    if (N <= 0 || nRows == 0 || nCols == 0) {
        outMat = inMat;
        return;
    }

    GenerateWindow(N);

    // 情况 1：列方向为脉冲维度
    if (static_cast<int>(nCols) == N) {
        for (size_t r = 0; r < nRows; ++r) {
            std::vector<std::complex<double>> x(N);
            for (int p = 0; p < N; ++p) x[p] = inMat(r, static_cast<size_t>(p));
            ProcessOneSlowTimeVector(x);
            for (int k = 0; k < N; ++k) outMat(r, static_cast<size_t>(k)) = x[k];
        }
        return;
    }

    // 情况 2：行方向为脉冲维度
    if (static_cast<int>(nRows) == N) {
        for (size_t c = 0; c < nCols; ++c) {
            std::vector<std::complex<double>> x(N);
            for (int p = 0; p < N; ++p) x[p] = inMat(static_cast<size_t>(p), c);
            ProcessOneSlowTimeVector(x);
            for (int k = 0; k < N; ++k) outMat(static_cast<size_t>(k), c) = x[k];
        }
        return;
    }

    // 情况 3：列方向可按 N 分块
    if (static_cast<int>(nCols) > N && static_cast<int>(nCols) % N == 0) {
        for (size_t r = 0; r < nRows; ++r) {
            for (size_t start = 0; start < nCols; start += static_cast<size_t>(N)) {
                std::vector<std::complex<double>> x(N);
                for (int p = 0; p < N; ++p) x[p] = inMat(r, start + static_cast<size_t>(p));
                ProcessOneSlowTimeVector(x);
                for (int k = 0; k < N; ++k) outMat(r, start + static_cast<size_t>(k)) = x[k];
            }
        }
        return;
    }

    // 情况 4：行方向可按 N 分块
    if (static_cast<int>(nRows) > N && static_cast<int>(nRows) % N == 0) {
        for (size_t c = 0; c < nCols; ++c) {
            for (size_t start = 0; start < nRows; start += static_cast<size_t>(N)) {
                std::vector<std::complex<double>> x(N);
                for (int p = 0; p < N; ++p) x[p] = inMat(start + static_cast<size_t>(p), c);
                ProcessOneSlowTimeVector(x);
                for (int k = 0; k < N; ++k) outMat(start + static_cast<size_t>(k), c) = x[k];
            }
        }
        return;
    }

    // 无法判断时保持输入不变
    outMat = inMat;
}

// ============================================================================
// DataStreamRun — 数据流模式
// ============================================================================

bool RADAR_MTD_M_Block::DataStreamRun()
{
    auto inputData = ReadInputData<SystemVueModelBuilder::DComplexMatrix>(GetInputPortName(0));
    if (inputData.empty()) return true;

    const auto& inMat = inputData[0];

    SystemVueModelBuilder::DComplexMatrix outMat;
    ProcessFrame(inMat, outMat);

    std::vector<SystemVueModelBuilder::DComplexMatrix> outVec;
    outVec.push_back(outMat);
    WriteOutputData(GetOutputPortName(0), outVec);
    return true;
}

// ============================================================================
// Initialize
// ============================================================================

bool RADAR_MTD_M_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_algo = std::make_unique<RADAR_MTD_M>();

    SetDefaultParameters();

    try { m_WindowType       = ConvertStringToWindowType(getParameter("WindowType").Value);            } catch (...) {}
    try { m_NumOfPulse       = std::stoi(getParameter("NumOfPulse").Value);                            } catch (...) {}
    try { m_Freq_WeightData      = ParseStringToDoubleVector(getParameter("Freq_Weight").Value);           } catch (...) {}
    try { m_WindowParametersData     = ParseStringToDoubleVector(getParameter("WindowParameters").Value);      } catch (...) {}

    SetParameters();

    AddInputPort("input",  m_algo->input,  1, Block::DataType::MATRIX_DCOMPLEX);
    AddOutputPort("output", m_algo->output, 1, Block::DataType::MATRIX_DCOMPLEX);

    return true;
}
