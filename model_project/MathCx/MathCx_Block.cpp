#include "MathCx_Block.h"

#include <algorithm>
#include <cctype>
#include <complex>
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

MathCx_Block::MathCx_Block(const std::string& name)
    : Block(name)
    , m_functionType(MathCx::Abs)
{
}

void MathCx_Block::SetDefaultParamters()
{
    m_functionType = MathCx::Abs;
}

void MathCx_Block::SetParameters(MathCx::SelectedFunctionType functionType)
{
    if (!m_mathCx) {
        return;
    }

    m_mathCx->FunctionType = functionType;
}

bool MathCx_Block::Setup()
{
    Block::Setup();
    return true;
}

bool MathCx_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_mathCx = std::make_unique<MathCx>();

    AddInputPort("input", m_mathCx->input, 1, Block::DataType::CIRCULAR_BUFFER_DCOMPLEX);
    AddOutputPort("output", m_mathCx->output, 1, Block::DataType::CIRCULAR_BUFFER_DCOMPLEX);

    SetDefaultParamters();

    try { m_functionType = ConvertStringToFunctionType(getParameter("FunctionType").Value); } catch (...) { }

    SetParameters(m_functionType);

    return true;
}

bool MathCx_Block::Run()
{
    if (!CanProcess()) {
        return false;
    }

    if (!m_mathCx) {
        return false;
    }

    const std::string inputPortName = GetInputPortName(0);
    const std::string outputPortName = GetOutputPortName(0);

    auto inputData = ReadInputData<std::complex<double>>(inputPortName);
    if (inputData.empty()) {
        return true;
    }

    std::vector<std::complex<double>> outputData;
    outputData.reserve(inputData.size());

    for (const auto& x : inputData) {
        std::complex<double> y(0.0, 0.0);
        switch (m_functionType)
        {
        case MathCx::Abs:
            y = std::complex<double>(std::abs(x), 0.0);
            break;
        case MathCx::Ceil:
            y.real(std::ceil(x.real()));
            y.imag(std::ceil(x.imag()));
            break;
        case MathCx::Exp:
            y = std::exp(x);
            break;
        case MathCx::Floor:
            y.real(std::floor(x.real()));
            y.imag(std::floor(x.imag()));
            break;
        case MathCx::Ln:
            y = std::log(x);
            break;
        case MathCx::Log10:
            y = std::log10(x);
            break;
        case MathCx::Pow10:
            y = std::pow(std::complex<double>(10.0, 0.0), x);
            break;
        case MathCx::Recip:
            y = std::pow(x, -1.0);
            break;
        case MathCx::Round:
            y.real(std::round(x.real()));
            y.imag(std::round(x.imag()));
            break;
        case MathCx::Sqr:
            y = std::pow(x, 2.0);
            break;
        case MathCx::Sqrt:
            y = std::sqrt(x);
            break;
        case MathCx::Conj:
            y = std::conj(x);
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

MathCx::SelectedFunctionType MathCx_Block::ConvertStringToFunctionType(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "abs") {
        return MathCx::Abs;
    }
    if (lower == "ceil" || lower == "1") return MathCx::Ceil;
    if (lower == "exp" || lower == "2") return MathCx::Exp;
    if (lower == "floor" || lower == "3") return MathCx::Floor;
    if (lower == "ln" || lower == "4") return MathCx::Ln;
    if (lower == "log10" || lower == "5") return MathCx::Log10;
    if (lower == "pow10" || lower == "6") return MathCx::Pow10;
    if (lower == "recip" || lower == "7") return MathCx::Recip;
    if (lower == "round" || lower == "8") return MathCx::Round;
    if (lower == "sqr" || lower == "9") return MathCx::Sqr;
    if (lower == "sqrt" || lower == "10") return MathCx::Sqrt;
    if (lower == "conj" || lower == "11") return MathCx::Conj;
    return MathCx::Abs;
}
