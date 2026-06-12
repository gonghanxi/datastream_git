#include "Math_Block.h"

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

Math_Block::Math_Block(const std::string& name)
    : Block(name)
    , m_functionType(Math::Abs)
{
}

void Math_Block::SetDefaultParamters()
{
    m_functionType = Math::Abs;
}

void Math_Block::SetParameters(Math::SelectedFunctionType functionType)
{
    if (!m_math) {
        return;
    }

    m_math->FunctionType = functionType;
}

bool Math_Block::Setup()
{
    Block::Setup();
    return true;
}

bool Math_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_math = std::make_unique<Math>();

    AddInputPort("input", m_math->input, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("output", m_math->output, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);

    SetDefaultParamters();

    try { m_functionType = ConvertStringToFunctionType(getParameter("FunctionType").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'FunctionType', using default value."); }

    SetParameters(m_functionType);

    return true;
}

bool Math_Block::Run()
{
    if (!m_math) {
        return false;
    }

    const std::string inputPortName = GetInputPortName(0);
    const std::string outputPortName = GetOutputPortName(0);

    auto inputData = ReadInputData<double>(inputPortName);
    if (inputData.empty()) {
        return true;
    }

    std::vector<double> outputData;
    outputData.reserve(inputData.size());

    for (double x : inputData) {
        double y = 0.0;
        switch (m_functionType)
        {
        case Math::Abs:
            y = std::fabs(x);
            break;
        case Math::Ceil:
            y = std::ceil(x);
            break;
        case Math::Exp:
            y = std::exp(x);
            break;
        case Math::Floor:
            y = std::floor(x);
            break;
        case Math::Ln:
            // 约束：自然对数输入必须 > 0
            if (x <= 0) {
                LOG_ERROR("自然对数(Ln)的输入值必须大于0，当前输入值: " + std::to_string(x));
                return false;
            }
            y = std::log(x);
            break;
        case Math::Log10:
            // 约束：常用对数输入必须 > 0
            if (x <= 0) {
                LOG_ERROR("常用对数(Log10)的输入值必须大于0，当前输入值: " + std::to_string(x));
                return false;
            }
            y = std::log10(x);
            break;
        case Math::Pow10:
            y = std::pow(10.0, x);
            break;
        case Math::Recip:
            // 约束：倒数输入不能为0
            if (x == 0) {
                LOG_ERROR("倒数(Recip)的输入值不能为0");
                return false;
            }
            y = 1.0 / x;
            break;
        case Math::Round:
            y = std::round(x);
            break;
        case Math::Sqr:
            y = x * x;
            break;
        case Math::Sqrt:
            // 约束：平方根输入必须 ≥ 0
            if (x < 0) {
                LOG_ERROR("平方根(Sqrt)的输入值不能小于0，当前输入值: " + std::to_string(x));
                return false;
            }
            y = std::sqrt(x);
            break;
        case Math::Sgn:
            if (x > 0) {
                y = 1.0;
            } else if (x < 0) {
                y = -1.0;
            } else {
                y = 0.0;
            }
            break;
        default:
            y = 0.0;
            break;
        }
        outputData.push_back(y);
    }

    WriteOutputData(outputPortName, outputData);
    return true;
}

Math::SelectedFunctionType Math_Block::ConvertStringToFunctionType(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "abs") {
        return Math::Abs;
    }
    if (lower == "ceil" || lower == "1") return Math::Ceil;
    if (lower == "exp" || lower == "2") return Math::Exp;
    if (lower == "floor" || lower == "3") return Math::Floor;
    if (lower == "ln" || lower == "4") return Math::Ln;
    if (lower == "log10" || lower == "5") return Math::Log10;
    if (lower == "pow10" || lower == "6") return Math::Pow10;
    if (lower == "recip" || lower == "7") return Math::Recip;
    if (lower == "round" || lower == "8") return Math::Round;
    if (lower == "sqr" || lower == "9") return Math::Sqr;
    if (lower == "sqrt" || lower == "10") return Math::Sqrt;
    if (lower == "sgn" || lower == "11") return Math::Sgn;
    return Math::Abs;
}
