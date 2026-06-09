#include "MxCom_M_Block.h"

#include <string>
#include <vector>

// ============================================================================
// 构造函数
// ============================================================================

MxCom_M_Block::MxCom_M_Block(const std::string& name)
    : Block(name)
    , m_OutputNumRows(100)
    , m_OutputNumCols(100)
    , m_InputNumRows(4)
    , m_InputNumCols(4)
{
}

// ============================================================================
// Setup
// ============================================================================

bool MxCom_M_Block::Setup()
{
    if (m_OutputNumRows < 1)
    {
        LOG_ERROR("OutputNumRows must be >= 1.");
        return false;
    }
    if (m_OutputNumCols < 1)
    {
        LOG_ERROR("OutputNumColumns must be >= 1.");
        return false;
    }
    if (m_InputNumRows < 1)
    {
        LOG_ERROR("NumRows must be >= 1.");
        return false;
    }
    if (m_InputNumCols < 1)
    {
        LOG_ERROR("NumCols must be >= 1.");
        return false;
    }
    if (m_OutputNumRows % m_InputNumRows != 0)
    {
        LOG_ERROR("OutputNumRows must be an integer multiple of InputNumRows.");
        return false;
    }
    if (m_OutputNumCols % m_InputNumCols != 0)
    {
        LOG_ERROR("OutputNumColumns must be an integer multiple of InputNumColumns.");
        return false;
    }

    Block::Setup();

    m_numSubMatrices = (m_OutputNumRows / m_InputNumRows) * (m_OutputNumCols / m_InputNumCols);
    m_inputBuffer.clear();
    while (!m_outputQueue.empty()) m_outputQueue.pop();

    return true;
}

// ============================================================================
// Run — 双模式分发
// ============================================================================

bool MxCom_M_Block::Run()
{
    if (IsVariableStepMode() || m_numSubMatrices > 1) { return TimeDrivenRun(); }
    return DataStreamRun();
}

// ============================================================================
// DataStreamRun：全量读取 → 一次合成输出
// ============================================================================

bool MxCom_M_Block::DataStreamRun()
{
    auto inputData = ReadInputData<SystemVueModelBuilder::DoubleMatrix>(GetInputPortName(0));
    if (inputData.empty()) { return true; }

    if (static_cast<int>(inputData.size()) < m_numSubMatrices) { return true; }

    SystemVueModelBuilder::DoubleMatrix outMx;
    outMx.Resize(m_OutputNumRows, m_OutputNumCols);

    for (int m = 0; m < m_OutputNumRows; ++m) {
        for (int n = 0; n < m_OutputNumCols; ++n) {
            const int MxRowIndex    = m / m_InputNumRows;
            const int MxColIndex    = n / m_InputNumCols;
            const int MxInputIndex  = MxRowIndex * (m_OutputNumCols / m_InputNumCols) + MxColIndex;
            const int SubMxRowIndex = m % m_InputNumRows;
            const int SubMxColIndex = n % m_InputNumCols;

            inputData[static_cast<size_t>(MxInputIndex)].Resize(m_InputNumRows, m_InputNumCols);
            outMx(m, n) = inputData[static_cast<size_t>(MxInputIndex)](SubMxRowIndex, SubMxColIndex);
        }
    }

    std::vector<SystemVueModelBuilder::DoubleMatrix> outputData;
    outputData.push_back(outMx);
    WriteOutputData(GetOutputPortName(0), outputData);

    return true;
}

// ============================================================================
// TimeDrivenRun：变步长逐点处理 — 累积子矩阵 → 满 numSubMatrices 后批次合成
// ============================================================================

bool MxCom_M_Block::TimeDrivenRun()
{
    // ① 累积子矩阵
    {
        auto inputData = ReadInputData<SystemVueModelBuilder::DoubleMatrix>(GetInputPortName(0));
        for (auto& v : inputData) m_inputBuffer.push_back(v);
    }

    // ② 缓冲区满 numSubMatrices → 批次合成入队
    if (static_cast<int>(m_inputBuffer.size()) >= m_numSubMatrices)
    {
        SystemVueModelBuilder::DoubleMatrix outMx;
        outMx.Resize(m_OutputNumRows, m_OutputNumCols);

        for (int m = 0; m < m_OutputNumRows; ++m) {
            for (int n = 0; n < m_OutputNumCols; ++n) {
                const int MxRowIndex    = m / m_InputNumRows;
                const int MxColIndex    = n / m_InputNumCols;
                const int MxInputIndex  = MxRowIndex * (m_OutputNumCols / m_InputNumCols) + MxColIndex;
                const int SubMxRowIndex = m % m_InputNumRows;
                const int SubMxColIndex = n % m_InputNumCols;

                m_inputBuffer[static_cast<size_t>(MxInputIndex)].Resize(m_InputNumRows, m_InputNumCols);
                outMx(m, n) = m_inputBuffer[static_cast<size_t>(MxInputIndex)](SubMxRowIndex, SubMxColIndex);
            }
        }

        m_outputQueue.push(outMx);
    }

    // ③ 出队写入一个输出矩阵，输出后清空输入缓冲区
    if (!m_outputQueue.empty()) {
        SystemVueModelBuilder::DoubleMatrix out = m_outputQueue.front(); m_outputQueue.pop();
        WriteOutputData(GetOutputPortName(0), std::vector<SystemVueModelBuilder::DoubleMatrix>{out});

        m_inputBuffer.clear();
    }

    return true;
}

// ============================================================================
// Initialize
// ============================================================================

bool MxCom_M_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_MxCom_M = std::make_unique<MxCom_M>();

    SetDefaultParameters();

    // 读取参数
    try { m_OutputNumRows = std::stoi(getParameter("OutputNumRows").Value); } catch (...) {}
    try { m_OutputNumCols = std::stoi(getParameter("OutputNumCols").Value); } catch (...) {}
    try { m_InputNumRows = std::stoi(getParameter("InputNumRows").Value); } catch (...) {}
    try { m_InputNumCols = std::stoi(getParameter("InputNumCols").Value); } catch (...) {}

    SetParameters();

    const int numSubMatrices = (m_OutputNumRows / m_InputNumRows) * (m_OutputNumCols / m_InputNumCols);

    AddInputPort("input", m_MxCom_M->input, static_cast<size_t>(numSubMatrices), Block::DataType::MATRIX_DOUBLE);
    AddOutputPort("output", m_MxCom_M->output, 1, Block::DataType::MATRIX_DOUBLE);

    return true;
}

// ============================================================================
// 参数设置
// ============================================================================

void MxCom_M_Block::SetDefaultParameters()
{
    m_OutputNumRows = 100;
    m_OutputNumCols = 100;
    m_InputNumRows = 4;
    m_InputNumCols = 4;
}

void MxCom_M_Block::SetParameters()
{
    if (!m_MxCom_M) return;
    m_MxCom_M->OutputNumRows = m_OutputNumRows;
    m_MxCom_M->OutputNumCols = m_OutputNumCols;
    m_MxCom_M->InputNumRows = m_InputNumRows;
    m_MxCom_M->InputNumCols = m_InputNumCols;
}
