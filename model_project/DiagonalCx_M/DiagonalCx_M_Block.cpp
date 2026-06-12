#include "DiagonalCx_M_Block.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <complex>
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

DiagonalCx_M_Block::DiagonalCx_M_Block(const std::string& name)
    : Block(name)
    , m_DiagonalElements()
    , m_ShowAdvancedParams(DiagonalCx_M::ShowAdv_No)
    , m_SampleRateOption(DiagonalCx_M::SRO_TimedFromSchematic)
    , m_SampleRate(1.0e6)
    , m_InitialDelay(0)
    , m_produced(0)
{
    m_DiagonalElements.Resize(1, 2);
    m_DiagonalElements(0) = { 1.0, 0.0 };
    m_DiagonalElements(1) = { 0.0, 1.0 };
}

// ============================================================================
// Setup
// ============================================================================

bool DiagonalCx_M_Block::Setup()
{
    Block::Setup();
    return true;
}

// ============================================================================
// Run
// ============================================================================

bool DiagonalCx_M_Block::Run()
{
    const int N = static_cast<int>(m_DiagonalElements.NumElements());
    if (N <= 0) {
        LOG_ERROR("DiagonalCx_M: DiagonalElements must contain at least one element.");
        return false;
    }

    // 构建对角矩阵
    DComplexMatrix diagMat;
    diagMat.Resize(N, N);
    for (int r = 0; r < N; ++r) {
        for (int c = 0; c < N; ++c) {
            diagMat(r, c) = (r == c) ? m_DiagonalElements(r) : std::complex<double>(0.0, 0.0);
        }
    }

    // 构建零矩阵
    DComplexMatrix zeroMat;
    zeroMat.Resize(N, N);
    for (int r = 0; r < N; ++r) {
        for (int c = 0; c < N; ++c) {
            zeroMat(r, c) = { 0.0, 0.0 };
        }
    }

    const bool adv = (m_ShowAdvancedParams == DiagonalCx_M::ShowAdv_Yes);

    const DComplexMatrix& y =
        (adv && (m_produced < m_InitialDelay)) ? zeroMat : diagMat;

    std::vector<DComplexMatrix> outputData;
    outputData.push_back(y);
    WriteOutputData(GetOutputPortName(0), outputData);

    ++m_produced;
    return true;
}

// ============================================================================
// Initialize
// ============================================================================

bool DiagonalCx_M_Block::Initialize()
{
    SetBlockType(Block::BlockType::SOURCE);

    m_DiagonalCx_M = std::make_unique<DiagonalCx_M>();

    SetDefaultParameters();

    // 读取参数
    try { m_ShowAdvancedParams = ConvertStringToShowAdvancedEnum(getParameter("ShowAdvancedParams").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'ShowAdvancedParams', using default value."); }
    try { m_SampleRateOption = ConvertStringToSampleRateOptionEnum(getParameter("SampleRateOption").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'SampleRateOption', using default value."); }
    try { m_SampleRate = std::stod(getParameter("SampleRate").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'SampleRate', using default value."); }
    try { m_InitialDelay = std::stoi(getParameter("InitialDelay").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'InitialDelay', using default value."); }

    // DiagonalElements 参数（复数矩阵类型）
    try { m_DiagonalElements = ParseStringToMatrix<std::complex<double>>(getParameter("DiagonalElements").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'DiagonalElements', using default value."); }

    SetParameters();

    AddOutputPort("output", m_DiagonalCx_M->output, 1, Block::DataType::MATRIX_TIME_DCOMPLEX);

    return true;
}

// ============================================================================
// 参数设置
// ============================================================================

void DiagonalCx_M_Block::SetDefaultParameters()
{
    m_DiagonalElements.Resize(1, 2);
    m_DiagonalElements(0) = { 1.0, 0.0 };
    m_DiagonalElements(1) = { 0.0, 1.0 };
    m_ShowAdvancedParams = DiagonalCx_M::ShowAdv_No;
    m_SampleRateOption = DiagonalCx_M::SRO_TimedFromSchematic;
    m_SampleRate = 1.0e6;
    m_InitialDelay = 0;
    m_produced = 0;
}

void DiagonalCx_M_Block::SetParameters()
{
    if (!m_DiagonalCx_M) return;
    m_DiagonalCx_M->DiagonalElements = m_DiagonalElements;
    m_DiagonalCx_M->ShowAdvancedParams = m_ShowAdvancedParams;
    m_DiagonalCx_M->SampleRateOption = m_SampleRateOption;
    m_DiagonalCx_M->SampleRate = m_SampleRate;
    m_DiagonalCx_M->InitialDelay = m_InitialDelay;
}

// ============================================================================
// 枚举转换
// ============================================================================

DiagonalCx_M::ShowAdvancedEnum
DiagonalCx_M_Block::ConvertStringToShowAdvancedEnum(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "no" || lower == "0") {
        return DiagonalCx_M::ShowAdv_No;
    }
    if (lower == "yes" || lower == "1") {
        return DiagonalCx_M::ShowAdv_Yes;
    }
    return DiagonalCx_M::ShowAdv_No;
}

DiagonalCx_M::SampleRateOptionEnum
DiagonalCx_M_Block::ConvertStringToSampleRateOptionEnum(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "untimed" || lower == "0") {
        return DiagonalCx_M::SRO_UnTimed;
    }
    if (lower == "timedfromsamplerate" || lower == "1") {
        return DiagonalCx_M::SRO_TimedFromSampleRate;
    }
    if (lower == "timedfromschematic" || lower == "2") {
        return DiagonalCx_M::SRO_TimedFromSchematic;
    }
    return DiagonalCx_M::SRO_TimedFromSchematic;
}
