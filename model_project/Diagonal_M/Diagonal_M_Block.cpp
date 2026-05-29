#include "Diagonal_M_Block.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <string>

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

Diagonal_M_Block::Diagonal_M_Block(const std::string& name)
    : Block(name)
    , m_DiagonalElements()
    , m_ShowAdvancedParams(Diagonal_M::ShowAdv_No)
    , m_SampleRateOption(Diagonal_M::SRO_TimedFromSchematic)
    , m_SampleRate(1.0e6)
    , m_InitialDelay(0)
    , m_produced(0)
{
    m_DiagonalElements.Resize(1, 2);
    m_DiagonalElements(0) = 1.0;
    m_DiagonalElements(1) = 2.0;
}

// ============================================================================
// Setup
// ============================================================================

bool Diagonal_M_Block::Setup()
{
    Block::Setup();
    return true;
}

// ============================================================================
// Run
// ============================================================================

bool Diagonal_M_Block::Run()
{
    const int N = static_cast<int>(m_DiagonalElements.NumElements());
    if (N <= 0) {
        LOG_ERROR("Diagonal_M: DiagonalElements must contain at least one element.");
        return false;
    }

    // 构建对角矩阵
    SystemVueModelBuilder::DoubleMatrix diagMat;
    diagMat.Resize(N, N);
    for (int r = 0; r < N; ++r) {
        for (int c = 0; c < N; ++c) {
            diagMat(r, c) = (r == c) ? m_DiagonalElements(r) : 0.0;
        }
    }

    // 构建零矩阵
    SystemVueModelBuilder::DoubleMatrix zeroMat;
    zeroMat.Resize(N, N);
    zeroMat.Zero();

    const bool adv = (m_ShowAdvancedParams == Diagonal_M::ShowAdv_Yes);

    const SystemVueModelBuilder::DoubleMatrix& y =
        (adv && (m_produced < m_InitialDelay)) ? zeroMat : diagMat;

    std::vector<SystemVueModelBuilder::DoubleMatrix> outputData;
    outputData.push_back(y);
    WriteOutputData(GetOutputPortName(0), outputData);

    ++m_produced;
    return true;
}

// ============================================================================
// Initialize
// ============================================================================

bool Diagonal_M_Block::Initialize()
{
    SetBlockType(Block::BlockType::SOURCE);

    m_Diagonal_M = std::make_unique<Diagonal_M>();

    SetDefaultParameters();

    // 读取参数
    try { m_ShowAdvancedParams = ConvertStringToShowAdvancedEnum(getParameter("ShowAdvancedParams").Value); } catch (...) {}
    try { m_SampleRateOption = ConvertStringToSampleRateOptionEnum(getParameter("SampleRateOption").Value); } catch (...) {}
    try { m_SampleRate = std::stod(getParameter("SampleRate").Value); } catch (...) {}
    try { m_InitialDelay = std::stoi(getParameter("InitialDelay").Value); } catch (...) {}

    // DiagonalElements 参数（矩阵类型）
    try { m_DiagonalElements = ParseStringToMatrix<double>(getParameter("DiagonalElements").Value); } catch (...) {}

    SetParameters();

    AddOutputPort("output", m_Diagonal_M->output, 1, Block::DataType::MATRIX_TIME_DOUBLE);

    return true;
}

// ============================================================================
// 参数设置
// ============================================================================

void Diagonal_M_Block::SetDefaultParameters()
{
    m_DiagonalElements.Resize(1, 2);
    m_DiagonalElements(0) = 1.0;
    m_DiagonalElements(1) = 2.0;
    m_ShowAdvancedParams = Diagonal_M::ShowAdv_No;
    m_SampleRateOption = Diagonal_M::SRO_TimedFromSchematic;
    m_SampleRate = 1.0e6;
    m_InitialDelay = 0;
    m_produced = 0;
}

void Diagonal_M_Block::SetParameters()
{
    if (!m_Diagonal_M) return;
    m_Diagonal_M->DiagonalElements = m_DiagonalElements;
    m_Diagonal_M->ShowAdvancedParams = m_ShowAdvancedParams;
    m_Diagonal_M->SampleRateOption = m_SampleRateOption;
    m_Diagonal_M->SampleRate = m_SampleRate;
    m_Diagonal_M->InitialDelay = m_InitialDelay;
}

// ============================================================================
// 枚举转换
// ============================================================================

Diagonal_M::ShowAdvancedEnum
Diagonal_M_Block::ConvertStringToShowAdvancedEnum(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "no" || lower == "0") {
        return Diagonal_M::ShowAdv_No;
    }
    if (lower == "yes" || lower == "1") {
        return Diagonal_M::ShowAdv_Yes;
    }
    return Diagonal_M::ShowAdv_No;
}

Diagonal_M::SampleRateOptionEnum
Diagonal_M_Block::ConvertStringToSampleRateOptionEnum(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "untimed" || lower == "0") {
        return Diagonal_M::SRO_UnTimed;
    }
    if (lower == "timedfromsamplerate" || lower == "1") {
        return Diagonal_M::SRO_TimedFromSampleRate;
    }
    if (lower == "timedfromschematic" || lower == "2") {
        return Diagonal_M::SRO_TimedFromSchematic;
    }
    return Diagonal_M::SRO_TimedFromSchematic;
}
