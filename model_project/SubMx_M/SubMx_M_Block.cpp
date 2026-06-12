#include "SubMx_M_Block.h"

// ============================================================================
// 构造函数
// ============================================================================

SubMx_M_Block::SubMx_M_Block(const std::string& name)
    : Block(name)
    , m_StartRow(1)
    , m_StartCol(1)
    , m_NumRows(2)
    , m_NumCols(2)
{
}

// ============================================================================
// Setup
// ============================================================================

bool SubMx_M_Block::Setup()
{
    Block::Setup();

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
    if (m_NumRows < 1)
    {
        LOG_ERROR("NumRows must be >= 1.");
        return false;
    }
    if (m_NumCols < 1)
    {
        LOG_ERROR("NumCols must be >= 1.");
        return false;
    }

    return true;
}

// ============================================================================
// Run
// ============================================================================

bool SubMx_M_Block::Run()
{
    auto inputData = ReadInputData<SystemVueModelBuilder::DoubleMatrix>(GetInputPortName(0));
    if (inputData.empty()) {
        return true;
    }

    const SystemVueModelBuilder::DoubleMatrix& inMat = inputData[0];

    if (m_StartRow + m_NumRows - 1 > inMat.NumRows())
    {
        LOG_ERROR("Input matrix is too small or sub matrix is too large to extract. StartRow + NumRows - 1 must be <= inputRows");
        return false;
    }
    if (m_StartCol + m_NumCols - 1 > inMat.NumColumns())
    {
        LOG_ERROR("Input matrix is too small or sub matrix is too large to extract. StartCol + NumCols - 1 must be <= inputCols");
        return false;
    }

    SystemVueModelBuilder::DoubleMatrix outMat;
    outMat.Resize(m_NumRows, m_NumCols);

    for (int m = 0; m < m_NumRows; ++m)
    {
        for (int n = 0; n < m_NumCols; ++n)
        {
            outMat(m, n) = inMat(m_StartRow + m - 1, m_StartCol + n - 1);
        }
    }

    std::vector<SystemVueModelBuilder::DoubleMatrix> outputData;
    outputData.push_back(outMat);
    WriteOutputData(GetOutputPortName(0), outputData);

    return true;
}

// ============================================================================
// Initialize
// ============================================================================

bool SubMx_M_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_SubMx_M = std::make_unique<SubMx_M>();

    SetDefaultParameters();

    try { m_StartRow = std::stoi(getParameter("StartRow").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'StartRow', using default value."); }
    try { m_StartCol = std::stoi(getParameter("StartCol").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'StartCol', using default value."); }
    try { m_NumRows  = std::stoi(getParameter("NumRows").Value);  } catch (...) { LOG_WARN("Failed to parse parameter 'NumRows', using default value."); }
    try { m_NumCols  = std::stoi(getParameter("NumCols").Value);  } catch (...) { LOG_WARN("Failed to parse parameter 'NumCols', using default value."); }

    SetParameters();

    AddInputPort("input", m_SubMx_M->input, 1, Block::DataType::MATRIX_DOUBLE);
    AddOutputPort("output", m_SubMx_M->output, 1, Block::DataType::MATRIX_DOUBLE);

    return true;
}

// ============================================================================
// 参数设置
// ============================================================================

void SubMx_M_Block::SetDefaultParameters()
{
    m_StartRow = 1;
    m_StartCol = 1;
    m_NumRows  = 2;
    m_NumCols  = 2;
}

void SubMx_M_Block::SetParameters()
{
    if (!m_SubMx_M) return;
    m_SubMx_M->StartRow = m_StartRow;
    m_SubMx_M->StartCol = m_StartCol;
    m_SubMx_M->NumRows  = m_NumRows;
    m_SubMx_M->NumCols  = m_NumCols;
}
