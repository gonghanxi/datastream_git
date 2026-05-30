#include "Limit_M_Block.h"

#include <algorithm>
#include <cctype>
#include <cmath>
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

Limit_M_Block::Limit_M_Block(const std::string& name)
    : Block(name)
    , m_K(1.0)
    , m_Bottom(0.0)
    , m_Top(1.0)
    , m_LimiterType(Limit_M::linear)
{
}

// ============================================================================
// Setup
// ============================================================================

bool Limit_M_Block::Setup()
{
    Block::Setup();
    return true;
}

// ============================================================================
// Run
// ============================================================================

bool Limit_M_Block::Run()
{
    // 读取输入矩阵
    auto inputData = ReadInputData<SystemVueModelBuilder::DoubleMatrix>(GetInputPortName(0));
    if (inputData.empty()) {
        return true;
    }

    const SystemVueModelBuilder::DoubleMatrix& inMx = inputData[0];
    const int numRow = inMx.NumRows();
    const int numCol = inMx.NumColumns();
    const int numElements = inMx.NumElements();

    const double PI = std::acos(-1.0);

    SystemVueModelBuilder::DoubleMatrix outMx;
    outMx.Resize(numRow, numCol);

    for (int i = 0; i < numElements; ++i)
    {
        if (m_LimiterType == Limit_M::linear)
        {
            if (inMx(i) < m_Bottom / m_K)
            {
                outMx(i) = m_Bottom;
            }
            else if (inMx(i) > m_Top / m_K)
            {
                outMx(i) = m_Top;
            }
            else
            {
                outMx(i) = m_K * inMx(i);
            }
        }

        if (m_LimiterType == Limit_M::atan)
        {
            outMx(i) = (m_Top - m_Bottom) / PI
                * std::atan(PI * (m_K * inMx(i) - (m_Top + m_Bottom) / 2.0) / (m_Top - m_Bottom))
                + (m_Top + m_Bottom) / 2.0;
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

bool Limit_M_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_Limit_M = std::make_unique<Limit_M>();

    SetDefaultParameters();

    // 读取参数
    try { m_K = std::stod(getParameter("K").Value); } catch (...) {}
    try { m_Bottom = std::stod(getParameter("Bottom").Value); } catch (...) {}
    try { m_Top = std::stod(getParameter("Top").Value); } catch (...) {}
    try { m_LimiterType = ConvertStringToSelectedLimiterType(getParameter("LimiterType").Value); } catch (...) {}

    SetParameters();

    if (m_K == 0)
    {
        LOG_ERROR("K must not be 0.");
        return false;
    }

    if (m_Bottom > m_Top)
    {
        LOG_ERROR("Top must be > Bottom.");
        return false;
    }

    AddInputPort("input", m_Limit_M->input, 1, Block::DataType::MATRIX_DOUBLE);
    AddOutputPort("output", m_Limit_M->output, 1, Block::DataType::MATRIX_DOUBLE);

    return true;
}

// ============================================================================
// 参数设置
// ============================================================================

void Limit_M_Block::SetDefaultParameters()
{
    m_K = 1.0;
    m_Bottom = 0.0;
    m_Top = 1.0;
    m_LimiterType = Limit_M::linear;
}

void Limit_M_Block::SetParameters()
{
    if (!m_Limit_M) return;
    m_Limit_M->K = m_K;
    m_Limit_M->Bottom = m_Bottom;
    m_Limit_M->Top = m_Top;
    m_Limit_M->LimiterType = m_LimiterType;
}

// ============================================================================
// 枚举转换
// ============================================================================

Limit_M::SelectedLimiterType
Limit_M_Block::ConvertStringToSelectedLimiterType(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "linear" || lower == "0") {
        return Limit_M::linear;
    }
    if (lower == "atan" || lower == "1") {
        return Limit_M::atan;
    }
    return Limit_M::linear;
}
