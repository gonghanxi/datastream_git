#include "RADAR_PdMeasure_Block.h"

#include <algorithm>
#include <cmath>
#include <string>

// ============================================================================
// 构造函数
// ============================================================================

RADAR_PdMeasure_Block::RADAR_PdMeasure_Block(const std::string& name)
    : Block(name)
    , m_PRI(1e-4)
    , m_SampleRate(10e6)
    , m_SimulationNumber(1000)
    , m_rangeBinNum(1000)
    , m_inputRate(1000000)
    , m_outputRate(1000)
{
}

// ============================================================================
// SetDefaultParameters
// ============================================================================

void RADAR_PdMeasure_Block::SetDefaultParameters()
{
    m_PRI              = 1e-4;
    m_SampleRate       = 10e6;
    m_SimulationNumber = 1000;
}

// ============================================================================
// SetParameters
// ============================================================================

void RADAR_PdMeasure_Block::SetParameters()
{
    if (!m_algo) return;
    m_algo->PRI              = m_PRI;
    m_algo->SampleRate       = m_SampleRate;
    m_algo->SimulationNumber = m_SimulationNumber;
}

// ============================================================================
// validateAndPrepare
// ============================================================================

bool RADAR_PdMeasure_Block::validateAndPrepare()
{
    if (!(m_PRI > 0.0) || !std::isfinite(m_PRI))
    {
        LOG_ERROR("PRI must be greater than 0.");
        return false;
    }
    if (!(m_SampleRate > 0.0) || !std::isfinite(m_SampleRate))
    {
        LOG_ERROR("SampleRate must be greater than 0.");
        return false;
    }
    if (m_SimulationNumber < 1)
    {
        LOG_ERROR("SimulationNumber must be greater than 0.");
        return false;
    }

    m_rangeBinNum = roundToInt(m_PRI * m_SampleRate);
    if (m_rangeBinNum < 1)
    {
        LOG_ERROR("PRI * SampleRate must be at least 1.");
        return false;
    }

    const long long inRate =
        static_cast<long long>(m_rangeBinNum) *
        static_cast<long long>(m_SimulationNumber);
    if (inRate <= 0 || inRate > 2147483647LL)
    {
        LOG_ERROR("Input rate PRI*SampleRate*SimulationNumber is too large.");
        return false;
    }

    m_inputRate  = static_cast<int>(inRate);
    m_outputRate = m_rangeBinNum;

    return true;
}

// ============================================================================
// Setup
// ============================================================================

bool RADAR_PdMeasure_Block::Setup()
{
    Block::Setup();

    m_inputBuffer.clear();
    while (!m_outputQueue.empty()) m_outputQueue.pop();

    return true;
}

// ============================================================================
// Run
// ============================================================================

bool RADAR_PdMeasure_Block::Run()
{
    if (IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

// ============================================================================
// Initialize
// ============================================================================

bool RADAR_PdMeasure_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_algo = std::make_unique<RADAR_PdMeasure>();

    SetDefaultParameters();

    try { m_PRI              = std::stod(getParameter("PRI").Value);              } catch (...) {}
    try { m_SampleRate       = std::stod(getParameter("SampleRate").Value);       } catch (...) {}
    try { m_SimulationNumber = std::stoi(getParameter("SimulationNumber").Value); } catch (...) {}

    SetParameters();

    if (!validateAndPrepare()) {
        return false;
    }

    AddInputPort("input",  m_algo->input,  m_inputRate,  Block::DataType::CIRCULAR_BUFFER_INT);
    AddOutputPort("output", m_algo->output, m_outputRate, Block::DataType::CIRCULAR_BUFFER_DOUBLE);

    return true;
}

// ============================================================================
// DataStreamRun
// ============================================================================

bool RADAR_PdMeasure_Block::DataStreamRun()
{
    auto inputData = ReadInputData<int>(GetInputPortName(0));
    if (inputData.empty()) return true;

    const int R = m_rangeBinNum;
    const double invSim = 1.0 / static_cast<double>(m_SimulationNumber);

    std::vector<double> outVec(static_cast<size_t>(R), 0.0);

    for (int r = 0; r < R; ++r)
    {
        int hitCount = 0;
        for (int s = 0; s < m_SimulationNumber; ++s)
        {
            const int idx = s * R + r;
            if (idx >= 0 && idx < m_inputRate)
            {
                if (inputData[static_cast<size_t>(idx)] != 0)
                    ++hitCount;
            }
        }
        outVec[static_cast<size_t>(r)] = clamp01(static_cast<double>(hitCount) * invSim);
    }

    WriteOutputData(GetOutputPortName(0), outVec);

    return true;
}

// ============================================================================
// TimeDrivenRun
// ============================================================================

bool RADAR_PdMeasure_Block::TimeDrivenRun()
{
    // ① 累积输入
    {
        auto inputData = ReadInputData<int>(GetInputPortName(0));
        for (auto& v : inputData) m_inputBuffer.push_back(v);
    }

    // ② 当累积足够时，处理一个块
    if (static_cast<int>(m_inputBuffer.size()) >= m_inputRate)
    {
        const int R = m_rangeBinNum;
        const double invSim = 1.0 / static_cast<double>(m_SimulationNumber);

        std::vector<double> outVec(static_cast<size_t>(R), 0.0);

        for (int r = 0; r < R; ++r)
        {
            int hitCount = 0;
            for (int s = 0; s < m_SimulationNumber; ++s)
            {
                const int idx = s * R + r;
                if (m_inputBuffer[static_cast<size_t>(idx)] != 0)
                    ++hitCount;
            }
            outVec[static_cast<size_t>(r)] = clamp01(static_cast<double>(hitCount) * invSim);
        }

        for (const auto& v : outVec) m_outputQueue.push(v);
        m_inputBuffer.clear();
    }

    // ③ 出队写入
    if (!m_outputQueue.empty())
    {
        double v = m_outputQueue.front(); m_outputQueue.pop();
        WriteOutputData(GetOutputPortName(0), std::vector<double>{v});
    }

    return true;
}

// ============================================================================
// 工具函数
// ============================================================================

int RADAR_PdMeasure_Block::roundToInt(double x)
{
    if (x != x || !std::isfinite(x))
        return 0;
    if (x >= 0.0)
        return static_cast<int>(std::floor(x + 0.5));
    return static_cast<int>(std::ceil(x - 0.5));
}

double RADAR_PdMeasure_Block::clamp01(double x)
{
    if (x != x || !std::isfinite(x))
        return 0.0;
    if (x < 0.0) return 0.0;
    if (x > 1.0) return 1.0;
    return x;
}
