#include "RADAR_CICDecimate_Block.h"

#include <algorithm>
#include <cmath>
#include <string>

// ============================================================================
// 构造函数
// ============================================================================

RADAR_CICDecimate_Block::RADAR_CICDecimate_Block(const std::string& name)
    : Block(name)
    , m_Order(5)
    , m_Ratio(2)
    , m_DiffDelay(1)
    , m_Phase(0)
    , m_cachedOrder(5)
    , m_cachedRatio(2)
    , m_cachedDiffDelay(1)
    , m_cachedPhase(0)
    , m_gainScale(1.0 / 32.0)
{
}

// ============================================================================
// SetDefaultParameters
// ============================================================================

void RADAR_CICDecimate_Block::SetDefaultParameters()
{
    m_Order     = 5;
    m_Ratio     = 2;
    m_DiffDelay = 1;
    m_Phase     = 0;
}

// ============================================================================
// SetParameters — 同步参数到算法对象
// ============================================================================

void RADAR_CICDecimate_Block::SetParameters()
{
    if (!m_algo) return;
    m_algo->Order     = m_Order;
    m_algo->Ratio     = m_Ratio;
    m_algo->DiffDelay = m_DiffDelay;
    m_algo->Phase     = m_Phase;
}

// ============================================================================
// Setup
// ============================================================================

bool RADAR_CICDecimate_Block::Setup()
{
    Block::Setup();

    if (!validateAndPrepare()) {
        return false;
    }

    resetStates();

    m_inputBuffer.clear();
    while (!m_outputQueue.empty()) m_outputQueue.pop();

    return true;
}

// ============================================================================
// Run — 双模式分发
// ============================================================================

bool RADAR_CICDecimate_Block::Run()
{
    if (IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

// ============================================================================
// Initialize
// ============================================================================

bool RADAR_CICDecimate_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_algo = std::make_unique<RADAR_CICDecimate>();

    SetDefaultParameters();

    try { m_Order     = std::stoi(getParameter("Order").Value);     } catch (...) { LOG_WARN("Failed to parse parameter 'Order', using default value."); }
    try { m_Ratio     = std::stoi(getParameter("Ratio").Value);     } catch (...) { LOG_WARN("Failed to parse parameter 'Ratio', using default value."); }
    try { m_DiffDelay = std::stoi(getParameter("DiffDelay").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'DiffDelay', using default value."); }
    try { m_Phase     = std::stoi(getParameter("Phase").Value);     } catch (...) { LOG_WARN("Failed to parse parameter 'Phase', using default value."); }

    SetParameters();

    const int rate = (m_Ratio > 0) ? m_Ratio : 2;

    AddInputPort("input",  m_algo->input,  rate, Block::DataType::CIRCULAR_BUFFER_DCOMPLEX);
    AddOutputPort("output", m_algo->output, 1,    Block::DataType::CIRCULAR_BUFFER_DCOMPLEX);

    return true;
}

// ============================================================================
// DataStreamRun — 数据流模式：处理一帧 Ratio 个样本 → 输出 1 个样本
// ============================================================================

bool RADAR_CICDecimate_Block::DataStreamRun()
{
    auto inputData = ReadInputData<std::complex<double>>(GetInputPortName(0));
    if (inputData.empty()) return true;

    const int takeIndex = m_cachedRatio - 1 - m_cachedPhase;
    Cx selected(0.0, 0.0);

    for (int r = 0; r < m_cachedRatio; ++r) {
        const Cx intOut = runIntegratorStages(inputData[static_cast<size_t>(r)]);
        if (r == takeIndex) {
            selected = intOut;
        }
    }

    const Cx combOut = runCombStages(selected);
    WriteOutputData(GetOutputPortName(0), std::vector<Cx>{combOut * m_gainScale});

    return true;
}

// ============================================================================
// TimeDrivenRun — 变步长模式：累积至 Ratio 个样本 → 处理 → 逐一出队
// ============================================================================

bool RADAR_CICDecimate_Block::TimeDrivenRun()
{
    // ① 累积输入
    {
        auto inputData = ReadInputData<std::complex<double>>(GetInputPortName(0));
        for (auto& v : inputData) m_inputBuffer.push_back(v);
    }

    // ② 当累积足够时，处理一个块
    if (static_cast<int>(m_inputBuffer.size()) >= m_cachedRatio) {
        const int takeIndex = m_cachedRatio - 1 - m_cachedPhase;
        Cx selected(0.0, 0.0);

        for (int r = 0; r < m_cachedRatio; ++r) {
            const Cx intOut = runIntegratorStages(m_inputBuffer[static_cast<size_t>(r)]);
            if (r == takeIndex) {
                selected = intOut;
            }
        }

        const Cx combOut = runCombStages(selected);
        m_outputQueue.push(combOut * m_gainScale);

        m_inputBuffer.clear();
    }

    // ③ 出队写入
    if (!m_outputQueue.empty()) {
        Cx v = m_outputQueue.front(); m_outputQueue.pop();
        WriteOutputData(GetOutputPortName(0), std::vector<Cx>{v});
    }

    return true;
}

// ============================================================================
// validateAndPrepare — 参数检查与内部缓存准备
// ============================================================================

bool RADAR_CICDecimate_Block::validateAndPrepare()
{
    m_cachedOrder     = m_Order;
    m_cachedRatio     = m_Ratio;
    m_cachedDiffDelay = m_DiffDelay;
    m_cachedPhase     = m_Phase;

    if (m_cachedOrder < 1)     m_cachedOrder = 1;
    if (m_cachedRatio < 1)     m_cachedRatio = 1;
    if (m_cachedDiffDelay < 1) m_cachedDiffDelay = 1;

    m_cachedPhase = clampInt(m_cachedPhase, 0, m_cachedRatio - 1);

    m_gainScale = computeGainScale();

    return true;
}

// ============================================================================
// resetStates — 重置积分器和 comb 延迟线状态
// ============================================================================

void RADAR_CICDecimate_Block::resetStates()
{
    m_integratorState.assign(static_cast<size_t>(m_cachedOrder), Cx(0.0, 0.0));

    m_combDelay.clear();
    m_combDelay.resize(static_cast<size_t>(m_cachedOrder));

    for (int s = 0; s < m_cachedOrder; ++s) {
        m_combDelay[static_cast<size_t>(s)].clear();
        for (int d = 0; d < m_cachedDiffDelay; ++d) {
            m_combDelay[static_cast<size_t>(s)].push_back(Cx(0.0, 0.0));
        }
    }
}

// ============================================================================
// runIntegratorStages — 高速积分器级联
// ============================================================================

RADAR_CICDecimate_Block::Cx
RADAR_CICDecimate_Block::runIntegratorStages(const Cx& x)
{
    Cx v = x;

    for (int s = 0; s < m_cachedOrder; ++s) {
        const size_t idx = static_cast<size_t>(s);
        m_integratorState[idx] += v;
        v = m_integratorState[idx];
    }

    return v;
}

// ============================================================================
// runCombStages — 低速 comb 延迟线级联
// ============================================================================

RADAR_CICDecimate_Block::Cx
RADAR_CICDecimate_Block::runCombStages(const Cx& x)
{
    Cx v = x;

    for (int s = 0; s < m_cachedOrder; ++s) {
        const size_t idx = static_cast<size_t>(s);

        Cx delayed(0.0, 0.0);
        if (!m_combDelay[idx].empty()) {
            delayed = m_combDelay[idx].front();
            m_combDelay[idx].pop_front();
        }

        m_combDelay[idx].push_back(v);

        v = v - delayed;
    }

    return v;
}

// ============================================================================
// computeGainScale — CIC 直流增益归一化
// ============================================================================

double RADAR_CICDecimate_Block::computeGainScale() const
{
    const double base = static_cast<double>(std::max(1, m_cachedRatio))
                      * static_cast<double>(std::max(1, m_cachedDiffDelay));

    double gain = 1.0;
    for (int i = 0; i < std::max(1, m_cachedOrder); ++i) {
        gain *= base;
    }

    if (gain <= 0.0) {
        return 1.0;
    }

    return 1.0 / gain;
}

// ============================================================================
// clampInt — 整数钳位
// ============================================================================

int RADAR_CICDecimate_Block::clampInt(int x, int lo, int hi)
{
    if (hi < lo) return lo;
    if (x < lo)  return lo;
    if (x > hi)  return hi;
    return x;
}
