#include "AvgSqrErr_M_Block.h"

// ============================================================================
// 构造函数
// ============================================================================

AvgSqrErr_M_Block::AvgSqrErr_M_Block(const std::string& name)
    : Block(name)
    , m_NumInputsToAverage(8)
    , m_rows(0)
    , m_cols(0)
    , m_shapeInit(false)
    , m_ring()
    , m_head(0)
    , m_accumSSE(0.0)
    , m_count(0)
{
}

// ============================================================================
// 默认参数
// ============================================================================

void AvgSqrErr_M_Block::SetDefaultParameters()
{
    m_NumInputsToAverage = 8;
    m_rows = 0;
    m_cols = 0;
    m_shapeInit = false;
    m_ring.clear();
    m_head = 0;
    m_accumSSE = 0.0;
    m_count = 0;
}

// ============================================================================
// 参数同步到算法实例
// ============================================================================

void AvgSqrErr_M_Block::SetParameters()
{
    if (!m_AvgSqrErr_M) return;
    m_AvgSqrErr_M->NumInputsToAverage = m_NumInputsToAverage;

    // 初始化滑动窗口
    if (m_AvgSqrErr_M->NumInputsToAverage > 0) {
        m_ring.assign(m_AvgSqrErr_M->NumInputsToAverage, 0.0);
    }
}

// ============================================================================
// Setup / Run / Initialize
// ============================================================================

bool AvgSqrErr_M_Block::Setup()
{
    Block::Setup();

    bool bStatus = true;

    if (m_NumInputsToAverage < 1)
    {
        LOG_ERROR("NumInputsToAverage must be >= 1");
        bStatus = false;
    }

    return bStatus;
}

bool AvgSqrErr_M_Block::Run()
{
    return DataStreamRun();
}

bool AvgSqrErr_M_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_AvgSqrErr_M = std::make_unique<AvgSqrErr_M>();
    SetDefaultParameters();

    // 读取参数
    try { m_NumInputsToAverage = std::stoi(getParameter("NumInputsToAverage").Value); } catch (...) {}

    SetParameters();

    if (!m_AvgSqrErr_M->Setup()) {
        LOG_ERROR("AvgSqrErr_M Setup failed");
        return false;
    }

    // 注册端口
    AddInputPort("input1", m_AvgSqrErr_M->input1, 1, Block::DataType::MATRIX_DOUBLE);
    AddInputPort("input2", m_AvgSqrErr_M->input2, 1, Block::DataType::MATRIX_DOUBLE);
    AddOutputPort("output", m_AvgSqrErr_M->output, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);

    return true;
}

// ============================================================================
// DataStreamRun：核心运行逻辑
// ============================================================================

bool AvgSqrErr_M_Block::DataStreamRun()
{
    SetParameters();

    // 读取两个输入矩阵
    auto input1Data = ReadInputData<SystemVueModelBuilder::Matrix<double>>(GetInputPortName(0));
    auto input2Data = ReadInputData<SystemVueModelBuilder::Matrix<double>>(GetInputPortName(1));

    if (input1Data.empty() || input2Data.empty()) return false;

    const SystemVueModelBuilder::Matrix<double>& A = input1Data[0];
    const SystemVueModelBuilder::Matrix<double>& B = input2Data[0];

    // 检查矩阵尺寸一致性
    if (!m_shapeInit) {
        m_rows = static_cast<int>(A.NumRows());
        m_cols = static_cast<int>(A.NumColumns());
        m_shapeInit = true;
    }

    if (A.NumRows() != static_cast<size_t>(m_rows) ||
        A.NumColumns() != static_cast<size_t>(m_cols) ||
        B.NumRows() != static_cast<size_t>(m_rows) ||
        B.NumColumns() != static_cast<size_t>(m_cols)) {
        LOG_ERROR("AvgSqrErr_M: input1 and input2 must keep identical sizes at every call.");
        std::vector<double> outputVec(1, 0.0);
        WriteOutputData(GetOutputPortName(0), outputVec);
        return false;
    }

    // 计算两个矩阵的平方误差和 (SSE)
    const size_t N = A.NumElements();
    double sse = 0.0;
    for (size_t i = 0; i < N; ++i) {
        const double d = A(i) - B(i);
        sse += d * d;
    }

    // 滑动窗口平均
    const bool window_full = (m_count >= m_NumInputsToAverage);
    const double oldest = window_full ? m_ring[m_head] : 0.0;

    if (!window_full) {
        ++m_count;
    }
    m_ring[m_head] = sse;
    m_head = (m_head + 1) % m_NumInputsToAverage;

    m_accumSSE += sse - oldest;

    const double avg = m_accumSSE / static_cast<double>(m_count);

    // 写入输出（标量）
    std::vector<double> outputVec(1, avg);
    WriteOutputData(GetOutputPortName(0), outputVec);

    return true;
}
