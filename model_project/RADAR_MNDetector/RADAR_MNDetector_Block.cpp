#include "RADAR_MNDetector_Block.h"

#include <cmath>
#include <string>

// ============================================================================
// 构造函数
// ============================================================================

RADAR_MNDetector_Block::RADAR_MNDetector_Block(const std::string& name)
    : Block(name)
    , m_M(2)
    , m_N(5)
    , m_PRI(1.0e-4)
    , m_SampleRate(10.0e6)
    , m_samplesPerPRI(1000)
    , m_inputRate(5000)
    , m_outputRate(1000)
{
}

// ============================================================================
// SetDefaultParameters
// ============================================================================

void RADAR_MNDetector_Block::SetDefaultParameters()
{
    m_M          = 2;
    m_N          = 5;
    m_PRI        = 1.0e-4;
    m_SampleRate = 10.0e6;
}

// ============================================================================
// SetParameters — 同步参数到算法对象
// ============================================================================

void RADAR_MNDetector_Block::SetParameters()
{
    if (!m_algo) return;
    m_algo->M          = m_M;
    m_algo->N          = m_N;
    m_algo->PRI        = m_PRI;
    m_algo->SampleRate = m_SampleRate;
}

// ============================================================================
// calcSamplesPerPRI — 计算每个 PRI 的采样点数（四舍五入）
// ============================================================================

int RADAR_MNDetector_Block::calcSamplesPerPRI(double pri, double sampleRate)
{
    const double v = pri * sampleRate;
    if (v <= 0.0) return 0;
    return static_cast<int>(std::floor(v + 0.5));
}

// ============================================================================
// validateAndPrepare — 参数检查与 rate 预计算
// ============================================================================

bool RADAR_MNDetector_Block::validateAndPrepare()
{
    if (m_M < 1) {
        LOG_ERROR("M must be >= 1.");
        return false;
    }
    if (m_N < 1) {
        LOG_ERROR("N must be >= 1.");
        return false;
    }
    if (m_M > m_N) {
        LOG_ERROR("M must be <= N.");
        return false;
    }
    if (m_PRI <= 0.0) {
        LOG_ERROR("PRI must be > 0.");
        return false;
    }
    if (m_SampleRate <= 0.0) {
        LOG_ERROR("SampleRate must be > 0.");
        return false;
    }

    m_samplesPerPRI = calcSamplesPerPRI(m_PRI, m_SampleRate);
    if (m_samplesPerPRI < 1) {
        LOG_ERROR("PRI * SampleRate must be >= 1 sample.");
        return false;
    }

    m_outputRate = m_samplesPerPRI;
    m_inputRate  = m_samplesPerPRI * m_N;

    if (m_inputRate < m_outputRate || m_inputRate <= 0 || m_outputRate <= 0) {
        LOG_ERROR("Invalid port rate calculated from PRI, SampleRate and N.");
        return false;
    }

    return true;
}

// ============================================================================
// Setup
// ============================================================================

bool RADAR_MNDetector_Block::Setup()
{
    Block::Setup();

    m_inputBuffer.clear();
    while (!m_outputQueue.empty()) m_outputQueue.pop();

    return true;
}

// ============================================================================
// Run — 双模式分发
// ============================================================================

bool RADAR_MNDetector_Block::Run()
{
    if (IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

// ============================================================================
// Initialize
// ============================================================================

bool RADAR_MNDetector_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_algo = std::make_unique<RADAR_MNDetector>();

    SetDefaultParameters();

    try { m_M          = std::stoi(getParameter("M").Value);          } catch (...) {}
    try { m_N          = std::stoi(getParameter("N").Value);          } catch (...) {}
    try { m_PRI        = std::stod(getParameter("PRI").Value);        } catch (...) {}
    try { m_SampleRate = std::stod(getParameter("SampleRate").Value); } catch (...) {}

    SetParameters();

    if (!validateAndPrepare()) {
        return false;
    }

    AddInputPort("input",  m_algo->input,  m_inputRate,  Block::DataType::CIRCULAR_BUFFER_INT);
    AddOutputPort("output", m_algo->output, m_outputRate, Block::DataType::CIRCULAR_BUFFER_INT);

    return true;
}

// ============================================================================
// DataStreamRun — 数据流模式：一次处理 N 个 PRI 的所有 range bin
// ============================================================================

bool RADAR_MNDetector_Block::DataStreamRun()
{
    auto inputData = ReadInputData<int>(GetInputPortName(0));
    if (inputData.empty()) return true;

    const int L = m_samplesPerPRI;
    std::vector<int> outVec(static_cast<size_t>(L), 0);

    for (int i = 0; i < L; ++i) {
        int hitCount = 0;
        for (int p = 0; p < m_N; ++p) {
            const int idx = p * L + i;
            if (inputData[static_cast<size_t>(idx)] == 1) {
                ++hitCount;
            }
        }
        outVec[static_cast<size_t>(i)] = (hitCount >= m_M) ? 1 : 0;
    }

    WriteOutputData(GetOutputPortName(0), outVec);

    return true;
}

// ============================================================================
// TimeDrivenRun — 变步长模式：累积 → 处理一个块 → 出队
// ============================================================================

bool RADAR_MNDetector_Block::TimeDrivenRun()
{
    // ① 累积输入
    {
        auto inputData = ReadInputData<int>(GetInputPortName(0));
        for (auto& v : inputData) m_inputBuffer.push_back(v);
    }

    // ② 当累积足够时，处理一个块
    if (static_cast<int>(m_inputBuffer.size()) >= m_inputRate) {
        const int L = m_samplesPerPRI;
        std::vector<int> outVec(static_cast<size_t>(L), 0);

        for (int i = 0; i < L; ++i) {
            int hitCount = 0;
            for (int p = 0; p < m_N; ++p) {
                const int idx = p * L + i;
                if (m_inputBuffer[static_cast<size_t>(idx)] == 1) {
                    ++hitCount;
                }
            }
            outVec[static_cast<size_t>(i)] = (hitCount >= m_M) ? 1 : 0;
        }

        for (const auto& v : outVec) m_outputQueue.push(v);
        m_inputBuffer.clear();
    }

    // ③ 出队写入
    if (!m_outputQueue.empty()) {
        int v = m_outputQueue.front(); m_outputQueue.pop();
        WriteOutputData(GetOutputPortName(0), std::vector<int>{v});
    }

    return true;
}
