#include "RADAR_CICInterp_Block.h"

#include <algorithm>
#include <cmath>
#include <string>

// ============================================================================
// 构造函数
// ============================================================================

RADAR_CICInterp_Block::RADAR_CICInterp_Block(const std::string& name)
    : Block(name)
    , m_Order(5)
    , m_Ratio(2)
    , m_DiffDelay(1)
    , m_Phase(0)
    , m_Fill(0.0, 0.0)
    , m_cachedOrder(5)
    , m_cachedRatio(2)
    , m_cachedDiffDelay(1)
    , m_cachedPhase(0)
    , m_gainScale(1.0 / 16.0)
{
}

// ============================================================================
// SetDefaultParameters
// ============================================================================

void RADAR_CICInterp_Block::SetDefaultParameters()
{
    m_Order     = 5;
    m_Ratio     = 2;
    m_DiffDelay = 1;
    m_Phase     = 0;
    m_Fill      = Cx(0.0, 0.0);
}

// ============================================================================
// SetParameters — 同步参数到算法对象
// ============================================================================

void RADAR_CICInterp_Block::SetParameters()
{
    if (!m_algo) return;
    m_algo->Order     = m_Order;
    m_algo->Ratio     = m_Ratio;
    m_algo->DiffDelay = m_DiffDelay;
    m_algo->Phase     = m_Phase;
    m_algo->Fill      = m_Fill;
}

// ============================================================================
// Setup
// ============================================================================

bool RADAR_CICInterp_Block::Setup()
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

bool RADAR_CICInterp_Block::Run()
{
    if (IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

// ============================================================================
// Initialize
// ============================================================================

bool RADAR_CICInterp_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_algo = std::make_unique<RADAR_CICInterp>();

    SetDefaultParameters();

    try { m_Order     = std::stoi(getParameter("Order").Value);     } catch (...) { LOG_WARN("Failed to parse parameter 'Order', using default value."); }
    try { m_Ratio     = std::stoi(getParameter("Ratio").Value);     } catch (...) { LOG_WARN("Failed to parse parameter 'Ratio', using default value."); }
    try { m_DiffDelay = std::stoi(getParameter("DiffDelay").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'DiffDelay', using default value."); }
    try { m_Phase     = std::stoi(getParameter("Phase").Value);     } catch (...) { LOG_WARN("Failed to parse parameter 'Phase', using default value."); }
    try { m_Fill      = Cx(std::stod(getParameter("Fill").Value), 0.0); } catch (...) { LOG_WARN("Failed to parse parameter 'Fill', using default value."); }

    SetParameters();

    AddInputPort("input",  m_algo->input,  1,          Block::DataType::CIRCULAR_BUFFER_DCOMPLEX);
    AddOutputPort("output", m_algo->output, m_Ratio,    Block::DataType::CIRCULAR_BUFFER_DCOMPLEX);

    return true;
}

// ============================================================================
// DataStreamRun — 数据流模式：1 输入 → comb → stuffer → integrator → Ratio 输出
// ============================================================================

bool RADAR_CICInterp_Block::DataStreamRun()
{
    auto inputData = ReadInputData<std::complex<double>>(GetInputPortName(0));
    if (inputData.empty()) return true;

    const Cx x = inputData[0];

    // 低速 comb 运行一次
    const Cx combOut = runCombStages(x);

    // specified-stuffer + 高速 integrator
    std::vector<Cx> outVec(static_cast<size_t>(m_cachedRatio));
    for (int r = 0; r < m_cachedRatio; ++r) {
        const Cx stuffed = (r == m_cachedPhase) ? combOut : m_Fill;
        outVec[static_cast<size_t>(r)] = runIntegratorStages(stuffed) * m_gainScale;
    }

    WriteOutputData(GetOutputPortName(0), outVec);

    return true;
}

// ============================================================================
// TimeDrivenRun — 变步长模式：累积 1 输入 → 处理 → Ratio 输出入队 → 逐一出队
// ============================================================================

bool RADAR_CICInterp_Block::TimeDrivenRun()
{
    // ① 累积输入
    {
        auto inputData = ReadInputData<std::complex<double>>(GetInputPortName(0));
        for (auto& v : inputData) m_inputBuffer.push_back(v);
    }

    // ② 处理所有累积的输入
    if (!m_inputBuffer.empty()) {
        const size_t count = m_inputBuffer.size();
        for (size_t i = 0; i < count; ++i) {
            const Cx x = m_inputBuffer[i];

            const Cx combOut = runCombStages(x);

            for (int r = 0; r < m_cachedRatio; ++r) {
                const Cx stuffed = (r == m_cachedPhase) ? combOut : m_Fill;
                m_outputQueue.push(runIntegratorStages(stuffed) * m_gainScale);
            }
        }
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

bool RADAR_CICInterp_Block::validateAndPrepare()
{
    if (m_Order <= 0) {
        LOG_ERROR("Order must be a positive integer.");
        return false;
    }
    if (m_Ratio <= 0) {
        LOG_ERROR("Ratio must be a positive integer.");
        return false;
    }
    if (m_DiffDelay <= 0) {
        LOG_ERROR("DiffDelay must be a positive integer.");
        return false;
    }
    m_cachedOrder     = m_Order;
    m_cachedRatio     = m_Ratio;
    m_cachedDiffDelay = m_DiffDelay;

    if (m_Phase < 0 || m_Phase >= m_cachedRatio) {
        LOG_ERROR("Phase must be in the range [0, Ratio-1].");
        return false;
    }
    m_cachedPhase = m_Phase;

    updateGainScale();

    return true;
}

// ============================================================================
// resetStates — 重置 comb 延迟线和 integrator 状态
// ============================================================================

void RADAR_CICInterp_Block::resetStates()
{
    m_combDelay.assign(static_cast<size_t>(m_cachedOrder),
        std::vector<Cx>(static_cast<size_t>(m_cachedDiffDelay), Cx(0.0, 0.0)));

    m_combWriteIndex.assign(static_cast<size_t>(m_cachedOrder), 0);
    m_integratorState.assign(static_cast<size_t>(m_cachedOrder), Cx(0.0, 0.0));
}

// ============================================================================
// updateGainScale — CIC 内插增益归一化
// gainScale = Ratio / (Ratio * DiffDelay)^Order
// ============================================================================

void RADAR_CICInterp_Block::updateGainScale()
{
    const double base = static_cast<double>(m_cachedRatio)
                      * static_cast<double>(m_cachedDiffDelay);

    if (m_cachedOrder <= 0 || base <= 0.0) {
        m_gainScale = 1.0;
        return;
    }

    const double rawGain = std::pow(base, static_cast<double>(m_cachedOrder))
                         / static_cast<double>(m_cachedRatio);

    if (rawGain > 0.0 && std::isfinite(rawGain))
        m_gainScale = 1.0 / rawGain;
    else
        m_gainScale = 1.0;
}

// ============================================================================
// runCombStages — N 级低速 comb（使用 writeIndex 环形缓冲区）
// ============================================================================

RADAR_CICInterp_Block::Cx
RADAR_CICInterp_Block::runCombStages(const Cx& x)
{
    Cx v = x;

    for (int s = 0; s < m_cachedOrder; ++s) {
        const size_t idx = static_cast<size_t>(s);
        const int   wi   = m_combWriteIndex[idx];
        const Cx    delayed = m_combDelay[idx][static_cast<size_t>(wi)];

        m_combDelay[idx][static_cast<size_t>(wi)] = v;
        m_combWriteIndex[idx] = (wi + 1) % m_cachedDiffDelay;

        v = v - delayed;
    }

    return v;
}

// ============================================================================
// runIntegratorStages — N 级高速积分器
// ============================================================================

RADAR_CICInterp_Block::Cx
RADAR_CICInterp_Block::runIntegratorStages(const Cx& x)
{
    Cx v = x;

    for (int s = 0; s < m_cachedOrder; ++s) {
        const size_t idx = static_cast<size_t>(s);
        m_integratorState[idx] += v;
        v = m_integratorState[idx];
    }

    return v;
}
