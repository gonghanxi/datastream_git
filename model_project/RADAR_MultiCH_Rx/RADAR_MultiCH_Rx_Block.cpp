#include "RADAR_MultiCH_Rx_Block.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <sstream>

// ============================================================================
// 构造函数
// ============================================================================

RADAR_MultiCH_Rx_Block::RADAR_MultiCH_Rx_Block(const std::string& name)
    : Block(name)
    , m_rng(1)
    , m_haveSpare(false)
    , m_spare(0.0)
    , m_sampleCount(0)
{
}

// ============================================================================
// Setup
// ============================================================================

bool RADAR_MultiCH_Rx_Block::Setup()
{
    Block::Setup();
    return true;
}

// ============================================================================
// Run — 分发
// ============================================================================

bool RADAR_MultiCH_Rx_Block::Run()
{
    if (IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

// ============================================================================
// DataStreamRun
// ============================================================================

bool RADAR_MultiCH_Rx_Block::DataStreamRun()
{
    using Cx = std::complex<double>;

    std::string inputPort  = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    // ---- 读取 EnvelopeSignal bus 输入 ----
    auto inputData = ReadInputData<EnvelopeSignal>(inputPort);
    if (inputData.empty()) return true;

    const int nChExpected = m_nChExpected;
    const int nInputCh = static_cast<int>(inputData.size());
    const int nUse = std::min(nChExpected, nInputCh);

    // ---- 获取 bus connections 用于逐通道 fc ----
    auto& conn = GetInputPort(inputPort)->GetBusConnections();

    const double fs = simulator_param.samplingRate;
    const double ts = (fs > 0.0) ? (1.0 / fs) : 0.0;
    const double st = simulator_param.startTime;
    const double t0 = st + static_cast<double>(m_sampleCount) * ts;

    // ---- step = N-2 ----
    long long step = static_cast<long long>(nChExpected) - 2LL;
    if (step < 0) step = 0;

    // ---- df_common & corr_common ----
    const double df_common = (fs > 0.0)
        ? std::remainder(static_cast<double>(step) * m_RefFreq, fs)
        : 0.0;
    const double cyc_common = std::remainder(df_common * t0, 1.0);
    const double ang_common = kTwoPi * cyc_common;
    const Cx corr_common(std::cos(ang_common), -std::sin(ang_common));

    // ---- 逐通道处理 ----
    std::vector<Cx> outputData;
    outputData.reserve(nChExpected);

    for (int k = 0; k < nUse; ++k)
    {
        // 逐通道获取 fc（原算法：input[k].GetCharacterizationFrequency()）
        double fc_in = conn.at(k).bridgeReader->getCharacterizationFrequency();
        if (fc_in < 0.0) fc_in = conn.at(0).bridgeReader->getCharacterizationFrequency();

        Cx x = inputData[k].complex();

        // ---- 频率旋转：Fc → Ref ----
        if (fc_in >= 0.0 && fs > 0.0)
        {
            const double t_fc = t0 + static_cast<double>(k) * ts;
            const double f_fc = std::remainder(fc_in, fs);
            const double cyc_fc = std::remainder(f_fc * t_fc, 1.0);
            const double ang_fc = kTwoPi * cyc_fc;
            const Cx rot_fc(std::cos(ang_fc), std::sin(ang_fc));

            const long long acc = static_cast<long long>(m_sampleCount) * step + static_cast<long long>(k);
            const double t_ref = st + static_cast<double>(acc) * ts;
            const double cyc_ref = std::remainder(m_RefFreq * t_ref, 1.0);
            const double ang_ref = kTwoPi * cyc_ref;
            const Cx rot_ref(std::cos(ang_ref), -std::sin(ang_ref));

            x *= (rot_fc * rot_ref);
        }

        // ---- 相位旋转 ----
        const double ph0 = deg2rad(m_phaseDeg[k]);
        const Cx ejph(std::cos(ph0), std::sin(ph0));
        Cx y = x * ejph;

        // ---- IQ 不平衡 + 通道系数 + 灵敏度 + 噪声 ----
        y = applyIQImbalance(y, m_iqGainDb[k], m_iqPhaseDeg[k]);
        y *= m_imbCoef[k];
        y *= m_sens[k];
        y += makeNoise(fs);

        // ---- 公共相位校正 ----
        y *= corr_common;

        outputData.push_back(y);
    }

    // 剩余通道填零
    for (int k = nUse; k < nChExpected; ++k)
        outputData.push_back(Cx(0.0, 0.0));

    WriteOutputData(outputPort, outputData);
    m_sampleCount++;

    return true;
}

// ============================================================================
// TimeDrivenRun — 逐点累积模式
// ============================================================================

bool RADAR_MultiCH_Rx_Block::TimeDrivenRun()
{
    using Cx = std::complex<double>;

    std::string inputPort  = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    auto inputData = ReadInputData<EnvelopeSignal>(inputPort);

    for (size_t i = 0; i < inputData.size(); ++i)
        m_inputBuffer.push_back(inputData[i]);

    if (static_cast<int>(m_inputBuffer.size()) >= m_nChExpected)
    {
        const int nUse = std::min(m_nChExpected, static_cast<int>(m_inputBuffer.size()));

        // ---- 获取 bus connections 用于逐通道 fc ----
        auto& conn = GetInputPort(inputPort)->GetBusConnections();

        const double fs = simulator_param.samplingRate;
        const double ts = (fs > 0.0) ? (1.0 / fs) : 0.0;
        const double st = simulator_param.startTime;
        const double t0 = st + static_cast<double>(m_sampleCount) * ts;

        long long step = static_cast<long long>(m_nChExpected) - 2LL;
        if (step < 0) step = 0;

        const double df_common = (fs > 0.0)
            ? std::remainder(static_cast<double>(step) * m_RefFreq, fs)
            : 0.0;
        const double cyc_common = std::remainder(df_common * t0, 1.0);
        const double ang_common = kTwoPi * cyc_common;
        const Cx corr_common(std::cos(ang_common), -std::sin(ang_common));

        for (int k = 0; k < nUse; ++k)
        {
            // 逐通道获取 fc（原算法：input[k].GetCharacterizationFrequency()）
            double fc_in = conn.at(k).bridgeReader->getCharacterizationFrequency();
            if (fc_in < 0.0) fc_in = conn.at(0).bridgeReader->getCharacterizationFrequency();

            Cx x = m_inputBuffer[k].complex();

            if (fc_in >= 0.0 && fs > 0.0)
            {
                const double t_fc = t0 + static_cast<double>(k) * ts;
                const double f_fc = std::remainder(fc_in, fs);
                const double cyc_fc = std::remainder(f_fc * t_fc, 1.0);
                const double ang_fc = kTwoPi * cyc_fc;
                const Cx rot_fc(std::cos(ang_fc), std::sin(ang_fc));

                const long long acc = static_cast<long long>(m_sampleCount) * step + static_cast<long long>(k);
                const double t_ref = st + static_cast<double>(acc) * ts;
                const double cyc_ref = std::remainder(m_RefFreq * t_ref, 1.0);
                const double ang_ref = kTwoPi * cyc_ref;
                const Cx rot_ref(std::cos(ang_ref), -std::sin(ang_ref));

                x *= (rot_fc * rot_ref);
            }

            const double ph0 = deg2rad(m_phaseDeg[k]);
            const Cx ejph(std::cos(ph0), std::sin(ph0));
            Cx y = x * ejph;

            y = applyIQImbalance(y, m_iqGainDb[k], m_iqPhaseDeg[k]);
            y *= m_imbCoef[k];
            y *= m_sens[k];
            y += makeNoise(fs);

            y *= corr_common;

            m_outputQueue.push(y);
        }

        // 剩余通道填零
        for (int k = nUse; k < m_nChExpected; ++k)
            m_outputQueue.push(Cx(0.0, 0.0));

        m_inputBuffer.clear();
        m_sampleCount++;
    }

    if (!m_outputQueue.empty())
    {
        Cx val = m_outputQueue.front();
        m_outputQueue.pop();

        std::vector<Cx> outputData;
        outputData.push_back(val);
        WriteOutputData(outputPort, outputData);
    }

    return true;
}

// ============================================================================
// Initialize
// ============================================================================

bool RADAR_MultiCH_Rx_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_algo = std::make_unique<RADAR_MultiCH_Rx>();

    // 获取仿真参数
    simulator_param = getSimu();

    // 解析参数
    SetDefaultParameters();
    try { m_RefFreq  = std::stod(getParameter("RefFreq").Value); } catch (...) {}
    try { m_NDensity = std::stod(getParameter("NDensity").Value); } catch (...) {}
    try { m_NumOfCh  = std::stoi(getParameter("NumOfCh").Value); } catch (...) {}

    if (m_NumOfCh < 1)
    {
        LOG_ERROR("RADAR_MultiCH_Rx: NumOfCh must be >= 1.");
        return false;
    }

    m_nChExpected = m_NumOfCh;

    rebuildCache();

    SetParameters();

    // 重置状态
    m_rng = std::mt19937(1);
    m_haveSpare = false;
    m_spare = 0.0;
    m_sampleCount = 0;

    AddInputPort("input",  m_algo->input,  1, Block::DataType::ENVELOPE_BUS);
    AddOutputPort("output", m_algo->output, 1, Block::DataType::DCOMPLEX_BUS);

    return true;
}

// ============================================================================
// SetDefaultParameters — 设置参数默认值
// ============================================================================

void RADAR_MultiCH_Rx_Block::SetDefaultParameters()
{
    m_RefFreq  = 1e6;
    m_NDensity = -173.975;
    m_NumOfCh  = 16;
}

// ============================================================================
// SetParameters — 将解析后的参数写入算法对象
// ============================================================================

void RADAR_MultiCH_Rx_Block::SetParameters()
{
    if (!m_algo) return;
    m_algo->RefFreq  = m_RefFreq;
    m_algo->NDensity = m_NDensity;
    m_algo->NumOfCh  = m_NumOfCh;
}

// ============================================================================
// rebuildCache
// ============================================================================

void RADAR_MultiCH_Rx_Block::rebuildCache()
{
    m_sens.assign(static_cast<size_t>(m_nChExpected), 1.0);
    m_phaseDeg.assign(static_cast<size_t>(m_nChExpected), 0.0);
    m_iqGainDb.assign(static_cast<size_t>(m_nChExpected), 0.0);
    m_iqPhaseDeg.assign(static_cast<size_t>(m_nChExpected), 0.0);
    m_imbCoef.assign(static_cast<size_t>(m_nChExpected), std::complex<double>(1.0, 0.0));

    // 解析 Sensitivity
    {
        std::vector<double> v;
        if (parseDoubleArray(getParameter("Sensitivity").Value, v))
        {
            const size_t n = std::min(v.size(), m_sens.size());
            for (size_t i = 0; i < n; ++i) m_sens[i] = v[i];
        }
    }
    // 解析 Phase
    {
        std::vector<double> v;
        if (parseDoubleArray(getParameter("Phase").Value, v))
        {
            const size_t n = std::min(v.size(), m_phaseDeg.size());
            for (size_t i = 0; i < n; ++i) m_phaseDeg[i] = v[i];
        }
    }
    // 解析 IQGainImbalance
    {
        std::vector<double> v;
        if (parseDoubleArray(getParameter("IQGainImbalance").Value, v))
        {
            const size_t n = std::min(v.size(), m_iqGainDb.size());
            for (size_t i = 0; i < n; ++i) m_iqGainDb[i] = v[i];
        }
    }
    // 解析 IQPhaseImbalance
    {
        std::vector<double> v;
        if (parseDoubleArray(getParameter("IQPhaseImbalance").Value, v))
        {
            const size_t n = std::min(v.size(), m_iqPhaseDeg.size());
            for (size_t i = 0; i < n; ++i) m_iqPhaseDeg[i] = v[i];
        }
    }
    // 解析 ImbalanceCoef
    {
        std::vector<std::complex<double>> v;
        if (parseComplexArray(getParameter("ImbalanceCoef").Value, v))
        {
            const size_t n = std::min(v.size(), m_imbCoef.size());
            for (size_t i = 0; i < n; ++i) m_imbCoef[i] = v[i];
        }
    }
}

// ============================================================================
// deg2rad
// ============================================================================

double RADAR_MultiCH_Rx_Block::deg2rad(double deg)
{
    return deg * kPi / 180.0;
}

// ============================================================================
// applyIQImbalance
// ============================================================================

std::complex<double> RADAR_MultiCH_Rx_Block::applyIQImbalance(
    const std::complex<double>& z, double gainDb, double phaseDeg)
{
    using Cx = std::complex<double>;

    const double g  = std::pow(10.0, gainDb / 20.0);
    const double ph = deg2rad(phaseDeg);

    const Cx ejph(std::cos(ph),  std::sin(ph));
    const Cx ejm(std::cos(ph), -std::sin(ph));

    const Cx alpha = 0.5 * (Cx(1.0, 0.0) + g * ejph);
    const Cx beta  = 0.5 * (Cx(1.0, 0.0) - g * ejm);

    return alpha * z + beta * std::conj(z);
}

// ============================================================================
// makeNoise
// ============================================================================

std::complex<double> RADAR_MultiCH_Rx_Block::makeNoise(double fs)
{
    using Cx = std::complex<double>;

    if (!(fs > 0.0))
        return Cx(0.0, 0.0);

    const double psd_W_per_Hz = 1e-3 * std::pow(10.0, m_NDensity / 10.0);
    const double R = 50.0;
    const double kVarCal = 8.0;
    const double var = psd_W_per_Hz * R * fs * kVarCal;
    const double sigma = std::sqrt(std::max(0.0, var / 2.0));

    std::normal_distribution<double> dist(0.0, sigma);
    return Cx(dist(m_rng), dist(m_rng));
}

// ============================================================================
// parseDoubleArray — 解析 "[1.0;2.0;3.0]" 或 "[1.0,2.0,3.0]" 格式
// ============================================================================

static std::string trim(const std::string& s)
{
    size_t start = s.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\n\r");
    return s.substr(start, end - start + 1);
}

bool RADAR_MultiCH_Rx_Block::parseDoubleArray(const std::string& str, std::vector<double>& out)
{
    out.clear();

    std::string s = trim(str);
    if (s.empty()) return false;

    if (s.front() != '[' || s.back() != ']') return false;
    s = s.substr(1, s.size() - 2);
    s = trim(s);
    if (s.empty()) return true;

    // 按逗号或分号分割
    std::vector<std::string> tokens;
    std::string token;
    for (size_t i = 0; i < s.size(); ++i)
    {
        char c = s[i];
        if (c == ',' || c == ';')
        {
            if (!token.empty())
            {
                tokens.push_back(token);
                token.clear();
            }
        }
        else
        {
            token += c;
        }
    }
    if (!token.empty())
        tokens.push_back(token);

    for (const auto& tok : tokens)
    {
        std::string t = trim(tok);
        if (t.empty()) continue;

        double val = 0.0;
        try { val = std::stod(t); } catch (...) { val = 0.0; }
        out.push_back(val);
    }

    return true;
}

// ============================================================================
// parseComplexArray — 解析 "[1+j*0, 0.5-j*0.3, ...]" 格式
// ============================================================================

bool RADAR_MultiCH_Rx_Block::parseComplexArray(const std::string& str, std::vector<std::complex<double>>& out)
{
    out.clear();

    std::string s = trim(str);
    if (s.empty()) return false;

    if (s.front() != '[' || s.back() != ']') return false;
    s = s.substr(1, s.size() - 2);
    s = trim(s);
    if (s.empty()) return true;

    // 按逗号分割
    std::vector<std::string> tokens;
    int depth = 0;
    std::string token;
    for (size_t i = 0; i < s.size(); ++i)
    {
        char c = s[i];
        if (c == '[' || c == '(') depth++;
        else if (c == ']' || c == ')') depth--;

        if (c == ',' && depth == 0)
        {
            tokens.push_back(token);
            token.clear();
        }
        else
        {
            token += c;
        }
    }
    if (!token.empty())
        tokens.push_back(token);

    for (const auto& tok : tokens)
    {
        std::string t = trim(tok);
        if (t.empty()) continue;

        double re = 0.0, im = 0.0;

        size_t jPos = t.find('j');
        if (jPos == std::string::npos) jPos = t.find('i');

        if (jPos != std::string::npos)
        {
            std::string reStr = t.substr(0, jPos);
            std::string imStr = t.substr(jPos + 1);

            {
                size_t starPos = imStr.find('*');
                if (starPos != std::string::npos)
                    imStr.erase(starPos, 1);
            }

            reStr = trim(reStr);
            imStr = trim(imStr);

            if (!reStr.empty())
            {
                if (reStr.back() == '+' || reStr.back() == '-')
                {
                    if (reStr.back() == '-') imStr = "-" + imStr;
                    reStr.pop_back();
                }
                reStr = trim(reStr);
                if (!reStr.empty())
                {
                    try { re = std::stod(reStr); } catch (...) { re = 0.0; }
                }
            }

            if (!imStr.empty())
            {
                try { im = std::stod(imStr); } catch (...) { im = 0.0; }
            }
        }
        else
        {
            try { re = std::stod(t); } catch (...) { re = 0.0; }
        }

        out.push_back(std::complex<double>(re, im));
    }

    return true;
}
