#include "RADAR_MultiCH_Tx_Block.h"

#include <algorithm>
#include <cmath>
#include <sstream>

// ============================================================================
// 构造函数
// ============================================================================

RADAR_MultiCH_Tx_Block::RADAR_MultiCH_Tx_Block(const std::string& name)
    : Block(name)
{
}

// ============================================================================
// Setup
// ============================================================================

bool RADAR_MultiCH_Tx_Block::Setup()
{
    Block::Setup();
    return true;
}

// ============================================================================
// Run — 分发
// ============================================================================

bool RADAR_MultiCH_Tx_Block::Run()
{
    if (IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

// ============================================================================
// DataStreamRun
// ============================================================================

bool RADAR_MultiCH_Tx_Block::DataStreamRun()
{
    using Cx = std::complex<double>;

    const int nChExpected = m_nChExpected;

    // ---- 读取 bus 输入 ----
    std::string inputPort = GetInputPortName(0);
    auto inputData = ReadInputData<Cx>(inputPort);

    const int nInputCh = static_cast<int>(inputData.size());

    // ---- 逐通道应用不平衡系数 ----
    std::string outputPort = GetOutputPortName(0);
    std::vector<EnvelopeSignal> outputData;
    outputData.reserve(nChExpected);

    for (int k = 0; k < nChExpected; ++k)
    {
        Cx x(0.0, 0.0);
        if (k < nInputCh)
            x = inputData[k];

        const Cx y = x * m_imbCache[k];
        outputData.push_back(EnvelopeSignal(y));
    }

    if (IsOutputBusToBus(outputPort)) {
        // bus-to-bus: 逐通道写入（带 fc）
        for (int k = 0; k < nChExpected; ++k) {
            std::vector<EnvelopeSignal> chData = {outputData[k]};
            GetOutputPort(outputPort)->WriteEnvelopeDataToChannel(k, chData, m_FCarrier);
        }
    } else {
        // bus-to-non-bus: 广播全部数据
        WriteOutputData(outputPort, outputData);
        auto& outConns = GetOutputPort(outputPort)->GetBusConnections();
        for (size_t k = 0; k < outConns.size(); ++k) {
            outConns.at(k).bridgeWriter->setCharacterizationFrequency(m_FCarrier);
        }
    }

    return true;
}

// ============================================================================
// TimeDrivenRun — 逐点累积模式
// ============================================================================

bool RADAR_MultiCH_Tx_Block::TimeDrivenRun()
{
    using Cx = std::complex<double>;

    std::string inputPort  = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    auto inputData = ReadInputData<Cx>(inputPort);

    for (size_t i = 0; i < inputData.size(); ++i)
        m_inputBuffer.push_back(inputData[i]);

    if (static_cast<int>(m_inputBuffer.size()) >= m_nChExpected)
    {
        const int nInputCh = std::min(m_nChExpected, static_cast<int>(m_inputBuffer.size()));

        for (int k = 0; k < m_nChExpected; ++k)
        {
            Cx x(0.0, 0.0);
            if (k < nInputCh)
                x = m_inputBuffer[k];

            const Cx y = x * m_imbCache[k];
            m_outputQueue.push(EnvelopeSignal(y));
        }

        m_inputBuffer.clear();
    }

    if (!m_outputQueue.empty())
    {
        if (IsOutputBusToBus(outputPort)) {
            // bus-to-bus: 一次输出 N 通道
            if (static_cast<int>(m_outputQueue.size()) >= m_nChExpected) {
                for (int k = 0; k < m_nChExpected; ++k) {
                    EnvelopeSignal val = m_outputQueue.front();
                    m_outputQueue.pop();
                    std::vector<EnvelopeSignal> chData = {val};
                    GetOutputPort(outputPort)->WriteEnvelopeDataToChannel(k, chData, m_FCarrier);
                }
            }
        } else {
            EnvelopeSignal val = m_outputQueue.front();
            m_outputQueue.pop();

            std::vector<EnvelopeSignal> outputData;
            outputData.push_back(val);
            WriteOutputData(outputPort, outputData);

            auto& outConns = GetOutputPort(outputPort)->GetBusConnections();
            for (size_t k = 0; k < outConns.size(); ++k) {
                outConns.at(k).bridgeWriter->setCharacterizationFrequency(m_FCarrier);
            }
        }
    }

    return true;
}

// ============================================================================
// Initialize
// ============================================================================

bool RADAR_MultiCH_Tx_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_algo = std::make_unique<RADAR_MultiCH_Tx>();

    // 解析参数
    SetDefaultParameters();
    try { m_NumOfCH = std::stoi(getParameter("NumOfCH").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'NumOfCH', using default value."); }
    try { m_TStep   = std::stod(getParameter("TStep").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'TStep', using default value."); }
    try { m_FCarrier = std::stod(getParameter("FCarrier").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'FCarrier', using default value."); }

    if (m_NumOfCH < 1)
    {
        LOG_ERROR("RADAR_MultiCH_Tx: NumOfCH must be >= 1.");
        return false;
    }

    m_nChExpected = m_NumOfCH;

    // 解析 ImbalanceCoef 并缓存
    rebuildCache();

    SetParameters();

    AddInputPort("input",  m_algo->input,  1, Block::DataType::DCOMPLEX_BUS);
    AddOutputPort("output", m_algo->output, 1, Block::DataType::ENVELOPE_BUS);

    return true;
}

// ============================================================================
// SetDefaultParameters — 设置参数默认值
// ============================================================================

void RADAR_MultiCH_Tx_Block::SetDefaultParameters()
{
    m_NumOfCH  = 16;
    m_TStep    = 1e-7;      // 1/10e6
    m_FCarrier = 1.0e9;
}

// ============================================================================
// SetParameters — 将解析后的参数写入算法对象
// ============================================================================

void RADAR_MultiCH_Tx_Block::SetParameters()
{
    if (!m_algo) return;
    m_algo->NumOfCH  = m_NumOfCH;
    m_algo->TStep    = m_TStep;
    m_algo->FCarrier = m_FCarrier;
}

// ============================================================================
// rebuildCache
// ============================================================================

void RADAR_MultiCH_Tx_Block::rebuildCache()
{
    m_imbCache.assign(static_cast<size_t>(m_nChExpected), std::complex<double>(1.0, 0.0));

    const std::string imbStr = getParameter("ImbalanceCoef").Value;
    std::vector<std::complex<double>> parsed;
    if (parseComplexArray(imbStr, parsed))
    {
        const size_t nFill = std::min(parsed.size(), m_imbCache.size());
        for (size_t k = 0; k < nFill; ++k)
            m_imbCache[k] = parsed[k];
    }
}

// ============================================================================
// parseComplexArray — 解析 "[1+j*0, 0.5-j*0.3, ...]" 格式
// ============================================================================

static std::string trim(const std::string& s)
{
    size_t start = s.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\n\r");
    return s.substr(start, end - start + 1);
}

bool RADAR_MultiCH_Tx_Block::parseComplexArray(const std::string& str, std::vector<std::complex<double>>& out)
{
    out.clear();

    std::string s = trim(str);
    if (s.empty()) return false;

    // 去除外层 [ ]
    if (s.front() != '[' || s.back() != ']') return false;
    s = s.substr(1, s.size() - 2);
    s = trim(s);
    if (s.empty()) return true; // 空数组

    // 按逗号分割，注意括号内可能有嵌套
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

    // 解析每个 token 为复数
    for (const auto& tok : tokens)
    {
        std::string t = trim(tok);
        if (t.empty()) continue;

        double re = 0.0, im = 0.0;
        bool hasIm = false;

        // 查找 'j' 或 'i' 标记虚部
        // 支持格式: "1+j*0", "1+0j", "0.5-j*0.3", "1", "1e3+j*2e3"
        size_t jPos = t.find('j');
        if (jPos == std::string::npos) jPos = t.find('i');

        if (jPos != std::string::npos)
        {
            hasIm = true;
            // 分离实部和虚部
            std::string reStr = t.substr(0, jPos);
            std::string imStr = t.substr(jPos + 1);

            // 去掉虚部的 '*' 号
            {
                size_t starPos = imStr.find('*');
                if (starPos != std::string::npos)
                    imStr.erase(starPos, 1);
            }

            reStr = trim(reStr);
            imStr = trim(imStr);

            // 实部：可能是 "1+", "1-", 或空（表示整个就是虚数如 "j*2"）
            if (!reStr.empty())
            {
                // 去掉末尾的 + 或 - 符号
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

            // 虚部
            if (!imStr.empty())
            {
                try { im = std::stod(imStr); } catch (...) { im = 0.0; }
            }
        }
        else
        {
            // 纯实数
            try { re = std::stod(t); } catch (...) { re = 0.0; }
        }

        out.push_back(std::complex<double>(re, im));
    }

    return true;
}
