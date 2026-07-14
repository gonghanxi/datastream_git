#include "MxDeCom_M_Block.h"

#include <string>
#include <vector>

// ============================================================================
// 构造函数
// ============================================================================

MxDeCom_M_Block::MxDeCom_M_Block(const std::string& name)
    : Block(name)
    , m_StartRow(1)
    , m_StartCol(1)
    , m_InputNumRows(100)
    , m_InputNumCols(100)
    , m_OutputNumRows(4)
    , m_OutputNumCols(4)
{
}

// ============================================================================
// Setup
// ============================================================================

bool MxDeCom_M_Block::Setup()
{
    if (m_StartRow < 1)
    {
        LOG_ERROR("StartRow must be >= 1.");
        return false;
    }
    if (m_StartCol < 1)
    {
        LOG_ERROR("StartCol must be >= 1.");
        return false;
    }
    if (m_InputNumRows < 1)
    {
        LOG_ERROR("InputNumRows must be >= 1.");
        return false;
    }
    if (m_InputNumCols < 1)
    {
        LOG_ERROR("InputNumCols must be >= 1.");
        return false;
    }
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
    if (m_InputNumRows % m_OutputNumRows != 0)
    {
        LOG_ERROR("InputNumRows must be an integer multiple of OutputNumRows.");
        return false;
    }
    if (m_InputNumCols % m_OutputNumCols != 0)
    {
        LOG_ERROR("InputNumColumns must be an integer multiple of OutputNumColumns.");
        return false;
    }

    Block::Setup();

    m_numSubMatrices = (m_InputNumRows / m_OutputNumRows) * (m_InputNumCols / m_OutputNumCols);
    m_inputBuffer.clear();
    m_outputQueue = std::queue<SystemVueModelBuilder::DoubleMatrix>();

    return true;
}

// ============================================================================
// Run
// ============================================================================

bool MxDeCom_M_Block::Run()
{
    if (IsVariableStepMode() && m_numSubMatrices > 1) { return TimeDrivenRun(); }
    return DataStreamRun();
}

// ============================================================================
// DataStreamRun：定步长单速率核心逻辑
// ============================================================================

bool MxDeCom_M_Block::DataStreamRun()
{
    auto inputData = ReadInputData<SystemVueModelBuilder::DoubleMatrix>(GetInputPortName(0));
    if (inputData.empty()) {
        return true;
    }

    const SystemVueModelBuilder::DoubleMatrix& inMx = inputData[0];

    // 原算法校验输入矩阵尺寸
    if (inMx.NumRows() < m_StartRow + m_InputNumRows - 1)
    {
        LOG_ERROR("Input matrix is too small. Rows of input matrix must >= StartRow + InputNumRows - 1.");
        return false;
    }
    if (inMx.NumColumns() < m_StartCol + m_InputNumCols - 1)
    {
        LOG_ERROR("Input matrix is too small. Columns of input matrix must >= StartCol + InputNumCols - 1.");
        return false;
    }

    std::vector<SystemVueModelBuilder::DoubleMatrix> outputData(m_numSubMatrices);

    for (int m = 0; m < m_InputNumRows; ++m)
    {
        for (int n = 0; n < m_InputNumCols; ++n)
        {
            int MxRowIndex = m / m_OutputNumRows;
            int MxColIndex = n / m_OutputNumCols;
            int MxOutputIndex = MxRowIndex * (m_InputNumCols / m_OutputNumCols) + MxColIndex;
            int SubMxRowIndex = m % m_OutputNumRows;
            int SubMxColIndex = n % m_OutputNumCols;

            outputData[MxOutputIndex].Resize(m_OutputNumRows, m_OutputNumCols);
            outputData[MxOutputIndex](SubMxRowIndex, SubMxColIndex) =
                inMx(m + m_StartRow - 1, n + m_StartCol - 1);
        }
    }

    WriteOutputData(GetOutputPortName(0), outputData);

    return true;
}

// ============================================================================
// TimeDrivenRun：变步长/多速率逐点处理 — 大矩阵拆分子矩阵
// ============================================================================

bool MxDeCom_M_Block::TimeDrivenRun()
{
    // ① 累积输入矩阵到 vector
    {
        auto inputData = ReadInputData<SystemVueModelBuilder::DoubleMatrix>(GetInputPortName(0));
        for (auto& v : inputData) m_inputBuffer.push_back(v);
    }

    // ② 有输入 → 分解为子矩阵入队
    if (!m_inputBuffer.empty())
    {
        const SystemVueModelBuilder::DoubleMatrix& inMx = m_inputBuffer[0];

        // 原算法校验输入矩阵尺寸
        if (inMx.NumRows() < m_StartRow + m_InputNumRows - 1)
        {
            LOG_ERROR("Input matrix is too small. Rows of input matrix must >= StartRow + InputNumRows - 1.");
            return false;
        }
        if (inMx.NumColumns() < m_StartCol + m_InputNumCols - 1)
        {
            LOG_ERROR("Input matrix is too small. Columns of input matrix must >= StartCol + InputNumCols - 1.");
            return false;
        }

        std::vector<SystemVueModelBuilder::DoubleMatrix> tempOutput(m_numSubMatrices);

        for (int m = 0; m < m_InputNumRows; ++m)
        {
            for (int n = 0; n < m_InputNumCols; ++n)
            {
                int MxRowIndex = m / m_OutputNumRows;
                int MxColIndex = n / m_OutputNumCols;
                int MxOutputIndex = MxRowIndex * (m_InputNumCols / m_OutputNumCols) + MxColIndex;
                int SubMxRowIndex = m % m_OutputNumRows;
                int SubMxColIndex = n % m_OutputNumCols;

                tempOutput[MxOutputIndex].Resize(m_OutputNumRows, m_OutputNumCols);
                tempOutput[MxOutputIndex](SubMxRowIndex, SubMxColIndex) =
                    inMx(m + m_StartRow - 1, n + m_StartCol - 1);
            }
        }

        for (auto& sub : tempOutput) m_outputQueue.push(sub);
        m_inputBuffer.clear();
    }

    // ③ 出队写入一个子矩阵
    if (!m_outputQueue.empty())
    {
        SystemVueModelBuilder::DoubleMatrix out = m_outputQueue.front();
        m_outputQueue.pop();
        std::vector<SystemVueModelBuilder::DoubleMatrix> outputVec;
        outputVec.push_back(out);
        WriteOutputData(GetOutputPortName(0), outputVec);
    }

    return true;
}

// ============================================================================
// Initialize
// ============================================================================

bool MxDeCom_M_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_MxDecom_M = std::make_unique<MxDecom_M>();

    SetDefaultParameters();

    // 读取参数
    try { m_StartRow = std::stoi(getParameter("StartRow").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'StartRow', using default value."); }
    try { m_StartCol = std::stoi(getParameter("StartCol").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'StartCol', using default value."); }
    try { m_InputNumRows = std::stoi(getParameter("InputNumRows").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'InputNumRows', using default value."); }
    try { m_InputNumCols = std::stoi(getParameter("InputNumCols").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'InputNumCols', using default value."); }
    try { m_OutputNumRows = std::stoi(getParameter("OutputNumRows").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'OutputNumRows', using default value."); }
    try { m_OutputNumCols = std::stoi(getParameter("OutputNumCols").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'OutputNumCols', using default value."); }

    SetParameters();

    const int numSubMatrices = (m_InputNumRows / m_OutputNumRows) * (m_InputNumCols / m_OutputNumCols);

    AddInputPort("input", m_MxDecom_M->input, 1, Block::DataType::MATRIX_DOUBLE);
    AddOutputPort("output", m_MxDecom_M->output, static_cast<size_t>(numSubMatrices), Block::DataType::MATRIX_DOUBLE);

    return true;
}

// ============================================================================
// 参数设置
// ============================================================================

void MxDeCom_M_Block::SetDefaultParameters()
{
    m_StartRow = 1;
    m_StartCol = 1;
    m_InputNumRows = 100;
    m_InputNumCols = 100;
    m_OutputNumRows = 4;
    m_OutputNumCols = 4;
}

void MxDeCom_M_Block::SetParameters()
{
    if (!m_MxDecom_M) return;
    m_MxDecom_M->StartRow = m_StartRow;
    m_MxDecom_M->StartCol = m_StartCol;
    m_MxDecom_M->InputNumRows = m_InputNumRows;
    m_MxDecom_M->InputNumCols = m_InputNumCols;
    m_MxDecom_M->OutputNumRows = m_OutputNumRows;
    m_MxDecom_M->OutputNumCols = m_OutputNumCols;
}
