#include "PattMatch_Block.h"

#include <cmath>
#include <limits>

// ============================================================================
// 构造函数
// ============================================================================

PattMatch_Block::PattMatch_Block(const std::string& name)
    : Block(name)
{
}

// ============================================================================
// Setup
// ============================================================================

bool PattMatch_Block::Setup()
{
    Block::Setup();
    while (!m_indexQueue.empty())  m_indexQueue.pop();
    while (!m_valuesQueue.empty()) m_valuesQueue.pop();
    m_templBuffer.clear();
    m_windowBuffer.clear();
    return true;
}

// ============================================================================
// Run — 分发
// ============================================================================

bool PattMatch_Block::Run()
{
    if (IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

// ============================================================================
// DataStreamRun — 批量模式
// ============================================================================

bool PattMatch_Block::DataStreamRun()
{
    std::string templPort  = GetInputPortName(0);
    std::string windowPort = GetInputPortName(1);
    std::string valuesPort  = GetOutputPortName(0);
    std::string indexPort = GetOutputPortName(1);

    auto templData  = ReadInputData<double>(templPort);
    auto windowData = ReadInputData<double>(windowPort);

    if (templData.empty() || windowData.empty()) return true;

    const int M = m_tempSize;
    const int N = m_winSize;
    const int numOut = N - M + 1;

    double maxCorr = std::numeric_limits<double>::lowest();
    int bestIndex = 0;

    std::vector<double> corrValues;

    for (int n = 0; n < numOut; ++n)
    {
        double num = 0.0;
        double den = 0.0;

        for (int m = 0; m < M; ++m)
        {
            const double t = templData[m];
            const double w = windowData[n + m];
            num += t * w;
            den += w * w;
        }

        double c = (den > 0.0) ? (num / den) : 0.0;
        corrValues.push_back(c);

        if (c > maxCorr)
        {
            maxCorr = c;
            bestIndex = n;
        }
    }

    WriteOutputData(valuesPort, corrValues);

    std::vector<int> indexData;
    indexData.push_back(bestIndex);
    WriteOutputData(indexPort, indexData);

    return true;
}

// ============================================================================
// TimeDrivenRun — 逐点累积模式
// ============================================================================

bool PattMatch_Block::TimeDrivenRun()
{
    std::string templPort  = GetInputPortName(0);
    std::string windowPort = GetInputPortName(1);
    std::string valuesPort  = GetOutputPortName(0);
    std::string indexPort = GetOutputPortName(1);

    auto templData  = ReadInputData<double>(templPort);
    auto windowData = ReadInputData<double>(windowPort);

    for (size_t i = 0; i < templData.size(); ++i)
        m_templBuffer.push_back(templData[i]);
    for (size_t i = 0; i < windowData.size(); ++i)
        m_windowBuffer.push_back(windowData[i]);

    if (static_cast<int>(m_templBuffer.size()) >= m_tempSize
        && static_cast<int>(m_windowBuffer.size()) >= m_winSize)
    {
        const int M = m_tempSize;
        const int N = m_winSize;
        const int numOut = N - M + 1;

        double maxCorr = std::numeric_limits<double>::lowest();
        int bestIndex = 0;

        std::vector<double> corrValues;

        for (int n = 0; n < numOut; ++n)
        {
            double num = 0.0;
            double den = 0.0;

            for (int m = 0; m < M; ++m)
            {
                const double t = m_templBuffer[m];
                const double w = m_windowBuffer[n + m];
                num += t * w;
                den += w * w;
            }

            double c = (den > 0.0) ? (num / den) : 0.0;
            corrValues.push_back(c);

            if (c > maxCorr)
            {
                maxCorr = c;
                bestIndex = n;
            }
        }

        m_indexQueue.push(bestIndex);
        m_valuesQueue.push(corrValues);

        m_templBuffer.clear();
        m_windowBuffer.clear();
    }

    if (!m_indexQueue.empty())
    {
        int val = m_indexQueue.front();
        m_indexQueue.pop();

        std::vector<int> indexData;
        indexData.push_back(val);
        WriteOutputData(indexPort, indexData);
    }
    if (!m_valuesQueue.empty())
    {
        std::vector<double> val = m_valuesQueue.front();
        m_valuesQueue.pop();

        WriteOutputData(valuesPort, val);
    }

    return true;
}

// ============================================================================
// Initialize
// ============================================================================

bool PattMatch_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_PattMatch = std::make_unique<PattMatch>();

    SetDefaultParameters();
    try { m_tempSize = std::stoi(getParameter("TempSize").Value); } catch (...) {}
    try { m_winSize  = std::stoi(getParameter("WinSize").Value);  } catch (...) {}
    SetParameters();

    if (m_tempSize <= 0)
    {
        LOG_ERROR("PattMatch: TempSize must be a positive integer.");
        return false;
    }
    if (m_winSize <= 0)
    {
        LOG_ERROR("PattMatch: WinSize must be a positive integer.");
        return false;
    }
    if (m_winSize < m_tempSize)
    {
        LOG_ERROR("PattMatch: WinSize must be >= TempSize.");
        return false;
    }

    const int valuesSize = m_winSize - m_tempSize + 1;

    AddInputPort("m_templ",  m_PattMatch->m_templ,  static_cast<size_t>(m_tempSize), Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddInputPort("m_window", m_PattMatch->m_window, static_cast<size_t>(m_winSize),  Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("m_values", m_PattMatch->m_values, static_cast<size_t>(valuesSize), Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("m_index",  m_indexPort,  1, Block::DataType::CIRCULAR_BUFFER_INT);

    return true;
}

// ============================================================================
// 参数设置
// ============================================================================

void PattMatch_Block::SetDefaultParameters()
{
    m_tempSize = 32;
    m_winSize  = 176;
}

void PattMatch_Block::SetParameters()
{
    if (!m_PattMatch) return;
    m_PattMatch->m_tempSize = m_tempSize;
    m_PattMatch->m_winSize  = m_winSize;
}
