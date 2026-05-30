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

    return true;
}

// ============================================================================
// Run
// ============================================================================

bool MxCom_M_Block::Run()
{
    auto inputData = ReadInputData<SystemVueModelBuilder::DoubleMatrix>(GetInputPortName(0));
    if (inputData.empty()) {
        return true;
    }

    const int numSubMatrices = (m_OutputNumRows / m_InputNumRows) * (m_OutputNumCols / m_InputNumCols);
    if (static_cast<int>(inputData.size()) < numSubMatrices) {
        return true;
    }

    // 合成输出矩阵
    SystemVueModelBuilder::DoubleMatrix outMx;
    outMx.Resize(m_OutputNumRows, m_OutputNumCols);

    for (int m = 0; m < m_OutputNumRows; ++m)
    {
        for (int n = 0; n < m_OutputNumCols; ++n)
        {
            int MxRowIndex = m / m_InputNumRows;
            int MxColIndex = n / m_InputNumCols;
            int MxInputIndex = MxRowIndex * (m_OutputNumCols / m_InputNumCols) + MxColIndex;
            int SubMxRowIndex = m % m_InputNumRows;
            int SubMxColIndex = n % m_InputNumCols;

            inputData[MxInputIndex].Resize(m_InputNumRows, m_InputNumCols);
            outMx(m, n) = inputData[MxInputIndex](SubMxRowIndex, SubMxColIndex);
        }
    }

    std::vector<SystemVueModelBuilder::DoubleMatrix> outputData;
    outputData.push_back(outMx);
    WriteOutputData(GetOutputPortName(0), outputData);

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
