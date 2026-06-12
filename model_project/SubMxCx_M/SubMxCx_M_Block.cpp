#include "SubMxCx_M_Block.h"

// ============================================================================
// 构造函数
// ============================================================================

SubMxCx_M_Block::SubMxCx_M_Block(const std::string& name)
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

bool SubMxCx_M_Block::Setup()
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

bool SubMxCx_M_Block::Run()
{
    auto inputData = ReadInputData<SystemVueModelBuilder::Matrix<std::complex<double>>>(GetInputPortName(0));
    if (inputData.empty()) {
        return true;
    }

    const SystemVueModelBuilder::Matrix<std::complex<double>>& inMat = inputData[0];

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

    SystemVueModelBuilder::Matrix<std::complex<double>> outMat;
    outMat.Resize(m_NumRows, m_NumCols);

    for (int m = 0; m < m_NumRows; ++m)
    {
        for (int n = 0; n < m_NumCols; ++n)
        {
            outMat(m, n) = inMat(m_StartRow + m - 1, m_StartCol + n - 1);
        }
    }

    std::vector<SystemVueModelBuilder::Matrix<std::complex<double>>> outputData;
    outputData.push_back(outMat);
    WriteOutputData(GetOutputPortName(0), outputData);

    return true;
}

// ============================================================================
// Initialize
// ============================================================================

bool SubMxCx_M_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_SubMxCx_M = std::make_unique<SubMxCx_M>();

    SetDefaultParameters();

    try { m_StartRow = std::stoi(getParameter("StartRow").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'StartRow', using default value."); }
    try { m_StartCol = std::stoi(getParameter("StartCol").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'StartCol', using default value."); }
    try { m_NumRows  = std::stoi(getParameter("NumRows").Value);  } catch (...) { LOG_WARN("Failed to parse parameter 'NumRows', using default value."); }
    try { m_NumCols  = std::stoi(getParameter("NumCols").Value);  } catch (...) { LOG_WARN("Failed to parse parameter 'NumCols', using default value."); }

    SetParameters();

    AddInputPort("input", m_SubMxCx_M->input, 1, Block::DataType::MATRIX_DCOMPLEX);
    AddOutputPort("output", m_SubMxCx_M->output, 1, Block::DataType::MATRIX_DCOMPLEX);

    return true;
}

// ============================================================================
// 参数设置
// ============================================================================

void SubMxCx_M_Block::SetDefaultParameters()
{
    m_StartRow = 1;
    m_StartCol = 1;
    m_NumRows  = 2;
    m_NumCols  = 2;
}

void SubMxCx_M_Block::SetParameters()
{
    if (!m_SubMxCx_M) return;
    m_SubMxCx_M->StartRow = m_StartRow;
    m_SubMxCx_M->StartCol = m_StartCol;
    m_SubMxCx_M->NumRows  = m_NumRows;
    m_SubMxCx_M->NumCols  = m_NumCols;
}
