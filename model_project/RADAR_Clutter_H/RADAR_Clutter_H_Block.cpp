#include "RADAR_Clutter_H_Block.h"

#include <algorithm>
#include <cctype>
#include <cmath>

// ============================================================================
// 枚举常量（映射到原算法 enum SelectedPDF / SelectedPSD）
// ============================================================================

namespace {
    // SelectedPDF
    constexpr int kPDF_Rayleigh  = 0;
    constexpr int kPDF_Lognormal = 1;
    constexpr int kPDF_Weibull   = 2;

    // SelectedPSD
    constexpr int kPSD_Gaussian = 0;
    constexpr int kPSD_Cauchy   = 1;
    constexpr int kPSD_Allpole  = 2;

    // 字符串处理
    std::string trim(const std::string& s)
    {
        size_t start = s.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) return "";
        size_t end = s.find_last_not_of(" \t\n\r");
        return s.substr(start, end - start + 1);
    }

    std::string toLower(const std::string& s)
    {
        std::string r = s;
        for (auto& c : r) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return r;
    }
}

// ============================================================================
// 构造函数
// ============================================================================

RADAR_Clutter_H_Block::RADAR_Clutter_H_Block(const std::string& name)
    : Block(name)
    , m_cachedNumSample(-1)
    , m_rng(std::random_device{}())
{
}

// ============================================================================
// Setup
// ============================================================================

bool RADAR_Clutter_H_Block::Setup()
{
    Block::Setup();
    return true;
}

// ============================================================================
// Run — 分发
// ============================================================================

bool RADAR_Clutter_H_Block::Run()
{
    if (IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

// ============================================================================
// DataStreamRun — 批量模式
// ============================================================================

bool RADAR_Clutter_H_Block::DataStreamRun()
{
    using Cx = std::complex<double>;

    std::string inputPort         = GetInputPortName(0);
    std::string outputPort        = GetOutputPortName(0);
    std::string clutterSamplePort = GetOutputPortName(1);
    std::string coeffPort         = GetOutputPortName(2);

    // ---- 读取全部输入 ----
    auto inputData = ReadInputData<EnvelopeSignal>(inputPort);
    if (inputData.empty()) return true;

    const int numSample = static_cast<int>(inputData.size());

    // ---- 生成杂波（缓存复用） ----
    if (m_cachedNumSample != numSample)
        generateClutter(numSample);

    // ---- 输出 = 输入 × 杂波 ----
    std::vector<EnvelopeSignal> outputData;
    std::vector<EnvelopeSignal> clutterData;
    outputData.reserve(numSample);
    clutterData.reserve(numSample);

    for (int i = 0; i < numSample; ++i)
    {
        const Cx x = inputData[i].complex();
        const Cx y = x * m_clutter[i];

        outputData.push_back(EnvelopeSignal(y));
        clutterData.push_back(EnvelopeSignal(m_clutter[i]));
    }

    WriteOutputData(outputPort, outputData);
    WriteOutputData(clutterSamplePort, clutterData);

    // ---- 写滤波器系数 ----
    std::vector<Cx> coeffData;
    coeffData.reserve(m_filterCoeff.size());
    for (size_t i = 0; i < m_filterCoeff.size(); ++i)
        coeffData.push_back(Cx(m_filterCoeff[i], 0.0));
    WriteOutputData(coeffPort, coeffData);

    return true;
}

// ============================================================================
// TimeDrivenRun — 逐点累积模式
// ============================================================================

bool RADAR_Clutter_H_Block::TimeDrivenRun()
{
    using Cx = std::complex<double>;

    std::string inputPort         = GetInputPortName(0);
    std::string outputPort        = GetOutputPortName(0);
    std::string clutterSamplePort = GetOutputPortName(1);
    std::string coeffPort         = GetOutputPortName(2);

    auto inputData = ReadInputData<EnvelopeSignal>(inputPort);

    for (size_t i = 0; i < inputData.size(); ++i)
        m_inputBuffer.push_back(inputData[i]);

    // ---- 全部输入收齐后生成杂波 ----
    const int numSample = static_cast<int>(m_DurationTime / m_TStep);
    if (static_cast<int>(m_inputBuffer.size()) >= numSample && m_cachedNumSample != numSample)
    {
        generateClutter(numSample);

        for (int i = 0; i < numSample; ++i)
        {
            const Cx x = m_inputBuffer[i].complex();
            const Cx y = x * m_clutter[i];
            m_outputQueue.push(EnvelopeSignal(y));
        }

        // 写滤波器系数
        std::vector<Cx> coeffData;
        coeffData.reserve(m_filterCoeff.size());
        for (size_t i = 0; i < m_filterCoeff.size(); ++i)
            coeffData.push_back(Cx(m_filterCoeff[i], 0.0));
        WriteOutputData(coeffPort, coeffData);

        // 写杂波样本
        for (int i = 0; i < numSample && i < static_cast<int>(m_inputBuffer.size()); ++i)
        {
            std::vector<EnvelopeSignal> cd;
            cd.push_back(EnvelopeSignal(m_clutter[i]));
            WriteOutputData(clutterSamplePort, cd);
        }

        m_inputBuffer.clear();
    }

    if (!m_outputQueue.empty())
    {
        EnvelopeSignal val = m_outputQueue.front();
        m_outputQueue.pop();

        std::vector<EnvelopeSignal> outputData;
        outputData.push_back(val);
        WriteOutputData(outputPort, outputData);
    }

    return true;
}

// ============================================================================
// Initialize
// ============================================================================

bool RADAR_Clutter_H_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_algo = std::make_unique<RADAR_Clutter_H>();

    simulator_param = getSimu();

    // 解析参数
    try { m_RF_Freq      = std::stod(getParameter("RF_Freq").Value); } catch (...) {}
    try { m_PDF          = ConvertStringToPDF(getParameter("PDF").Value); } catch (...) {}
    try { m_VA           = std::stod(getParameter("VA").Value); } catch (...) {}
    try { m_VB           = std::stod(getParameter("VB").Value); } catch (...) {}
    try { m_PSD          = ConvertStringToPSD(getParameter("PSD").Value); } catch (...) {}
    try { m_PA           = std::stod(getParameter("PA").Value); } catch (...) {}
    try { m_PB           = std::stod(getParameter("PB").Value); } catch (...) {}
    try { m_TStep        = std::stod(getParameter("TStep").Value); } catch (...) {}
    try { m_FilterLen    = std::stoi(getParameter("FilterLen").Value); } catch (...) {}
    try { m_DurationTime = std::stod(getParameter("DurationTime").Value); } catch (...) {}
    try { m_Vr           = std::stod(getParameter("Vr").Value); } catch (...) {}

    if (m_FilterLen <= 0)
    {
        LOG_ERROR("RADAR_Clutter_H: FilterLen must be > 0.");
        return false;
    }
    if (m_TStep <= 0)
    {
        LOG_ERROR("RADAR_Clutter_H: TStep must be > 0.");
        return false;
    }
    if (m_TStep > m_DurationTime)
    {
        LOG_ERROR("RADAR_Clutter_H: TStep must be <= DurationTime.");
        return false;
    }

    m_cachedNumSample = -1;
    m_rng = std::mt19937(std::random_device{}());

    AddInputPort("input",          m_algo->input,         1, Block::DataType::ENVELOPE_SIGNAL);
    AddOutputPort("output",        m_algo->output,        1, Block::DataType::ENVELOPE_SIGNAL);
    AddOutputPort("ClutterSample", m_algo->ClutterSample, 1, Block::DataType::ENVELOPE_SIGNAL);
    AddOutputPort("Coeff",         m_algo->Coeff,         1, Block::DataType::DCOMPLEX_BUS);

    return true;
}

// ============================================================================
// ConvertStringToPDF
// ============================================================================

int RADAR_Clutter_H_Block::ConvertStringToPDF(const std::string& value)
{
    const std::string lower = toLower(trim(value));
    if (lower == "rayleigh"  || lower == "rayleigh pdf"  || lower == "0") return kPDF_Rayleigh;
    if (lower == "lognormal" || lower == "lognormal pdf" || lower == "1") return kPDF_Lognormal;
    if (lower == "weibull"   || lower == "weibull pdf"   || lower == "2") return kPDF_Weibull;
    return kPDF_Rayleigh;
}

// ============================================================================
// ConvertStringToPSD
// ============================================================================

int RADAR_Clutter_H_Block::ConvertStringToPSD(const std::string& value)
{
    const std::string lower = toLower(trim(value));
    if (lower == "gaussian"  || lower == "gaussian psd"  || lower == "0") return kPSD_Gaussian;
    if (lower == "cauchy"    || lower == "cauchy psd"    || lower == "1") return kPSD_Cauchy;
    if (lower == "allpole"   || lower == "allpole psd"   || lower == "2") return kPSD_Allpole;
    return kPSD_Gaussian;
}

// ============================================================================
// generateGaussianPSD — 生成高斯 PSD 滤波器系数（新版 b_half + 折叠）
// ============================================================================

void RADAR_Clutter_H_Block::generateGaussianPSD(double fr)
{
    const double c = 3e8;
    const double lambda0 = c / m_RF_Freq;
    const double sigmav = m_PA;
    const double sigmaf = 2.0 * sigmav / lambda0;

    const int coe_num = m_FilterLen / 2;

    // b_half
    std::vector<double> b_half(coe_num, 0.0);
    for (int n = 0; n < coe_num; ++n)
    {
        b_half[n] = 2.0 * sigmaf * std::sqrt(kPi)
            * std::exp(-4.0 * sigmaf * sigmaf * kPi * kPi
                       * static_cast<double>(n * n) / (fr * fr)) / fr;
    }

    // 对称折叠到 b
    m_filterCoeff.assign(m_FilterLen, 0.0);
    for (int n = 0; n < m_FilterLen; ++n)
    {
        if (n < coe_num)
            m_filterCoeff[n] = 0.5 * b_half[coe_num - n - 1];
        else
            m_filterCoeff[n] = 0.5 * b_half[n - coe_num];
    }
}

// ============================================================================
// generateClutter — 生成完整杂波序列
// ============================================================================

void RADAR_Clutter_H_Block::generateClutter(int numSample)
{
    using Cx = std::complex<double>;

    const double fr = 1.0 / m_DurationTime;

    // ---- 高斯白噪声（Box-Muller 方法，与原始算法一致） ----
    std::uniform_real_distribution<double> dist01(0.0, 1.0);
    std::vector<double> xi(numSample), xq(numSample);
    for (int i = 0; i < numSample; ++i)
    {
        const double u1 = dist01(m_rng);
        const double u2 = dist01(m_rng);
        xi[i] = 2.0 * std::sqrt(-2.0 * std::log(u1)) * std::cos(2.0 * kPi * u2);
        xq[i] = 2.0 * std::sqrt(-2.0 * std::log(u1)) * std::sin(2.0 * kPi * u2);
    }

    // ---- PSD 滤波（线性卷积） ----
    generateGaussianPSD(fr);

    std::vector<double> xiconv = linearConvolve(m_filterCoeff, xi);  // FilterLen × numSample
    std::vector<double> xqconv = linearConvolve(m_filterCoeff, xq);

    // 去暂态响应：截取 result[i + FilterLen - 1]
    std::vector<double> xxi(numSample), xxq(numSample);
    for (int i = 0; i < numSample; ++i)
    {
        xxi[i] = xiconv[i + m_FilterLen - 1];
        xxq[i] = xqconv[i + m_FilterLen - 1];
    }

    // ---- 归一化（标准化为零均值、单位方差） ----
    const double xim = average(xxi);
    const double xqm = average(xxq);
    const double xis = stddev(xxi);
    const double xqs = stddev(xxq);

    std::vector<double> yyi(numSample), yyq(numSample);
    for (int i = 0; i < numSample; ++i)
    {
        yyi[i] = (xxi[i] - xim) / xis;
        yyq[i] = (xxq[i] - xqm) / xqs;
    }

    // ---- PDF 成形 ----
    m_clutter.assign(numSample, Cx(0.0, 0.0));

    switch (m_PDF)
    {
    case kPDF_Rayleigh:
    {
        const double sigmac = m_VA;
        for (int i = 0; i < numSample; ++i)
        {
            m_clutter[i] = Cx(sigmac * yyi[i], sigmac * yyq[i]);
        }
        break;
    }
    case kPDF_Lognormal:
    {
        const double sigmac = m_VA;
        const double muc = m_VB;
        for (int i = 0; i < numSample; ++i)
        {
            m_clutter[i] = std::exp(Cx(sigmac * yyi[i] + std::log(muc),
                                       sigmac * yyq[i] + std::log(muc)));
        }
        break;
    }
    case kPDF_Weibull:
    {
        const double p = m_VA;
        const double q = m_VB;
        const double sigmac = std::sqrt(std::pow(q, p) / 2.0);
        for (int i = 0; i < numSample; ++i)
        {
            const double ri = sigmac * yyi[i];
            const double rq = sigmac * yyq[i];
            m_clutter[i] = Cx(std::pow(ri * ri, 1.0 / p),
                              std::pow(rq * rq, 1.0 / p));
        }
        break;
    }
    default:
        break;
    }

    m_cachedNumSample = numSample;
}

// ============================================================================
// 统计辅助函数
// ============================================================================

double RADAR_Clutter_H_Block::average(const std::vector<double>& a)
{
    if (a.empty()) return 0.0;
    double sum = 0.0;
    for (const auto& v : a) sum += v;
    return sum / static_cast<double>(a.size());
}

double RADAR_Clutter_H_Block::variance(const std::vector<double>& a)
{
    if (a.empty()) return 0.0;
    const double avg = average(a);
    double sum = 0.0;
    for (const auto& v : a) sum += (v - avg) * (v - avg);
    return sum / static_cast<double>(a.size());
}

double RADAR_Clutter_H_Block::stddev(const std::vector<double>& a)
{
    return std::sqrt(variance(a));
}

// ============================================================================
// linearConvolve — 线性卷积（a × b，返回大小 = lenA + lenB - 1）
// ============================================================================

std::vector<double> RADAR_Clutter_H_Block::linearConvolve(const std::vector<double>& a, const std::vector<double>& b)
{
    const int lenA = static_cast<int>(a.size());
    const int lenB = static_cast<int>(b.size());
    if (lenA <= 0 || lenB <= 0) return {};

    std::vector<double> result(lenA + lenB - 1, 0.0);
    for (int i = 0; i < lenA; ++i)
    {
        for (int j = 0; j < lenB; ++j)
        {
            result[i + j] += a[i] * b[j];
        }
    }
    return result;
}
