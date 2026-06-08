#include "RealToInt_Block.h"

#include <algorithm>
#include <cmath>
#include <cctype>
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

RealToInt_Block::RealToInt_Block(const std::string& name)
    : Block(name)
{
}

// ============================================================================
// 默认参数
// ============================================================================

void RealToInt_Block::SetDefaultParameters()
{
    m_convertType = RealToInt::Static_Cast;
}

// ============================================================================
// 参数同步到算法实例
// ============================================================================

void RealToInt_Block::SetParameters()
{
    if (!m_algo) { return; }

    m_algo->ConvertType = m_convertType;
}

// ============================================================================
// Setup / Run / Initialize
// ============================================================================

bool RealToInt_Block::Setup()
{
    Block::Setup();
    return true;
}

bool RealToInt_Block::Run()
{
    return DataStreamRun();
}

bool RealToInt_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_algo = std::make_unique<RealToInt>();
    SetDefaultParameters();

    // ---- 读取参数 ----
    try { m_convertType = ConvertStringToConvertType(getParameter("ConvertType").Value); } catch (...) {}

    SetParameters();

    // ---- 注册端口 ----
    AddInputPort("input",  m_algo->input,  1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("output", m_algo->output, 1, Block::DataType::CIRCULAR_BUFFER_INT);

    return true;
}

// ============================================================================
// DataStreamRun：核心运行逻辑（double 转 int）
// ============================================================================

bool RealToInt_Block::DataStreamRun()
{
    auto inputData = ReadInputData<double>(GetInputPortName(0));
    if (inputData.empty()) { return false; }

    // 类型转换：double -> int（根据 ConvertType）
    std::vector<int> outputData;
    outputData.reserve(inputData.size());

    for (size_t i = 0; i < inputData.size(); ++i) {
        switch (m_convertType) {
        case RealToInt::Static_Cast:
            outputData.push_back(static_cast<int>(inputData[i]));
            break;
        case RealToInt::Floor:
            outputData.push_back(static_cast<int>(std::floor(inputData[i])));
            break;
        case RealToInt::Ceil:
            outputData.push_back(static_cast<int>(std::ceil(inputData[i])));
            break;
        case RealToInt::Round:
            outputData.push_back(static_cast<int>(std::round(inputData[i])));
            break;
        default:
            outputData.push_back(static_cast<int>(inputData[i]));
            break;
        }
    }

    WriteOutputData(GetOutputPortName(0), outputData);

    return true;
}

// ============================================================================
// 枚举转换
// ============================================================================

RealToInt::SelectedConvertType RealToInt_Block::ConvertStringToConvertType(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "static_cast" || lower == "0") {
        return RealToInt::Static_Cast;
    }
    if (lower == "floor" || lower == "1") {
        return RealToInt::Floor;
    }
    if (lower == "ceil" || lower == "2") {
        return RealToInt::Ceil;
    }
    if (lower == "round" || lower == "3") {
        return RealToInt::Round;
    }
    return RealToInt::Static_Cast;
}
