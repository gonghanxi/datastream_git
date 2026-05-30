#include "PackBus_M_Block.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

namespace {
std::string TrimCopy(const std::string& value)
{
    std::string s = value;
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
    return s;
}

std::string ToLowerCopy(const std::string& value)
{
    std::string s = value;
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return s;
}
}

// ============================================================================
// 构造函数
// ============================================================================

PackBus_M_Block::PackBus_M_Block(const std::string& name)
    : Block(name)
    , m_NumRows(1)
    , m_NumCols(1)
    , m_Format(PackBus_M::ColumnMajor)
{
}

// ============================================================================
// Setup
// ============================================================================

bool PackBus_M_Block::Setup()
{
    Block::Setup();
    return true;
}

// ============================================================================
// Run
// ============================================================================

bool PackBus_M_Block::Run()
{
    auto inputData = ReadInputData<double>(GetInputPortName(0));

    const int expectedSize = m_NumRows * m_NumCols;
    if (static_cast<int>(inputData.size()) < expectedSize) {
        return true;
    }

    SystemVueModelBuilder::DoubleMatrix outMx;
    outMx.Resize(m_NumRows, m_NumCols);

    for (int row = 0; row < m_NumRows; ++row)
    {
        for (int col = 0; col < m_NumCols; ++col)
        {
            // Format==0: ColumnMajor → inputIndex = col * NumRows + row
            // Format==1: RowMajor    → inputIndex = row * NumCols + col
            int inputIndex = m_Format ? (row * m_NumCols + col) : (col * m_NumRows + row);
            outMx(row, col) = inputData[inputIndex];
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

bool PackBus_M_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_PackBus_M = std::make_unique<PackBus_M>();

    SetDefaultParameters();

    try { m_NumRows = std::stoi(getParameter("NumRows").Value); } catch (...) {}
    try { m_NumCols = std::stoi(getParameter("NumCols").Value); } catch (...) {}
    try { m_Format = ConvertStringToFormat(getParameter("Format").Value); } catch (...) {}

    SetParameters();

    if (m_NumRows < 1 || m_NumCols < 1)
    {
        LOG_ERROR("NumRows and NumCols must be >= 1.");
        return false;
    }

    AddInputPort("input", m_PackBus_M->input, 1, Block::DataType::DOUBLE_BUS);
    AddOutputPort("output", m_PackBus_M->output, 1, Block::DataType::MATRIX_DOUBLE);

    return true;
}

// ============================================================================
// 参数设置
// ============================================================================

void PackBus_M_Block::SetDefaultParameters()
{
    m_NumRows = 1;
    m_NumCols = 1;
    m_Format = PackBus_M::ColumnMajor;
}

void PackBus_M_Block::SetParameters()
{
    if (!m_PackBus_M) return;
    m_PackBus_M->NumRows = m_NumRows;
    m_PackBus_M->NumCols = m_NumCols;
    m_PackBus_M->Format = m_Format;
}

// ============================================================================
// 枚举转换
// ============================================================================

PackBus_M::SelectedFormat PackBus_M_Block::ConvertStringToFormat(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "columnmajor" || lower == "0") {
        return PackBus_M::ColumnMajor;
    }
    if (lower == "rowmajor" || lower == "1") {
        return PackBus_M::RowMajor;
    }
    return PackBus_M::ColumnMajor;
}
