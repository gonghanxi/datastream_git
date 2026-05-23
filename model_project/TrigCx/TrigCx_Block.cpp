#include "TrigCx_Block.h"
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
TrigCx_Block::TrigCx_Block(const std::string &name)
    :Block(name)
{

}

bool TrigCx_Block::Setup()
{
    Block::Setup();
    return true;
}

bool TrigCx_Block::Run()
{
    if (!CanProcess()) {
        return false;
    }

    std::string inputPort = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    auto inputData = ReadInputData<std::complex<double>>(inputPort);
    if (inputData.empty()) {
        return false;
    }
    std::vector<std::complex<double>> outputData(inputData.size());

    bool bStatus = true;
    for(size_t i = 0; i < inputData.size(); i++) {
        switch (m_FunctionType)
        {
        case TrigCx::Sin:
            outputData[i] = std::sin(inputData[i]);
            break;
        case TrigCx::Cos:
            outputData[i] = std::cos(inputData[i]);
            break;
        case TrigCx::Tan:
            outputData[i] = std::tan(inputData[i]);
            break;
        case TrigCx::Cot:
            if (inputData[i] == 0.0)
            {
                LOG_ERROR("Cot input is out of domain.(z|z≠0)");
                bStatus = false;
            }
            else
            {
                outputData[i] = 1.0 / std::tan(inputData[i]);
            }
            break;
        case TrigCx::Asin:
            outputData[i] = std::asin(inputData[i]);
            break;
        case TrigCx::Acos:
            outputData[i] = std::acos(inputData[i]);
            break;
        case TrigCx::Atan:
            outputData[i] = std::atan(inputData[i]);
            break;
        case TrigCx::Acot:
            if (inputData[i] == 0.0)
            {
                LOG_ERROR("Acot input is out of domain.(z|z≠0)");
                bStatus = false;
            }
            else
            {
                outputData[i] = std::atan(1.0 / inputData[i]);
            }
            break;
        case TrigCx::Sinh:
            outputData[i] = std::sinh(inputData[i]);
            break;
        case TrigCx::Cosh:
            outputData[i] = std::cosh(inputData[i]);
            break;
        case TrigCx::Tanh:
            outputData[i] = std::tanh(inputData[i]);
            break;
        case TrigCx::Coth:
            if (outputData[i] == 0.0)
            {
                LOG_ERROR("Coth input is out of domain.(z|z≠0)");
                bStatus = false;
            }
            else
            {
                outputData[i] = 1.0 / std::tanh(inputData[i]);
            }
            break;
        case TrigCx::Asinh:
            outputData[i] = std::asinh(inputData[i]);
            break;
        case TrigCx::Acosh:
            outputData[i] = std::acosh(inputData[i]);
            break;
        case TrigCx::Atanh:
            outputData[i] = std::atanh(inputData[i]);
            break;
        case TrigCx::Acoth:
            if (inputData[i] == 0.0)
            {
                LOG_ERROR("Acoth input is out of domain.(z|z≠0)");
                bStatus = false;
            }
            else
            {
                outputData[i] = std::atanh(1.0 / inputData[i]);
            }
            break;
        default:
            break;
        }
    }

    WriteOutputData(outputPort, outputData);
    return bStatus;
}

bool TrigCx_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_TrigCx = std::make_unique<TrigCx>();

    SetDefaultParameters();

    try { m_FunctionType = ConvertStringToSelectedFunctionType(getParameter("FunctionType").Value); } catch (...) { }

    SetParameters();

    AddInputPort("input", m_TrigCx->input, 1, DataType::CIRCULAR_BUFFER_DCOMPLEX);
    AddOutputPort("output", m_TrigCx->output, 1, DataType::CIRCULAR_BUFFER_DCOMPLEX);

    return true;
}

void TrigCx_Block::SetParameters()
{
    if(!m_TrigCx) return;
    m_TrigCx->FunctionType = m_FunctionType;
}

TrigCx::SelectedFunctionType TrigCx_Block::ConvertStringToSelectedFunctionType(const std::string &value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "sin" || lower == "0") {
        return TrigCx::Sin;
    }
    if (lower == "cos" || lower == "1") {
        return TrigCx::Cos;
    }
    if (lower == "tan" || lower == "2") {
        return TrigCx::Tan;
    }
    if (lower == "cot" || lower == "3") {
        return TrigCx::Cot;
    }
    if (lower == "asin" || lower == "4") {
        return TrigCx::Asin;
    }
    if (lower == "acos" || lower == "5") {
        return TrigCx::Acos;
    }
    if (lower == "atan" || lower == "6") {
        return TrigCx::Atan;
    }
    if (lower == "acot" || lower == "7") {
        return TrigCx::Acot;
    }
    if (lower == "sinh" || lower == "8") {
        return TrigCx::Sinh;
    }
    if (lower == "cosh" || lower == "9") {
        return TrigCx::Cosh;
    }
    if (lower == "tanh" || lower == "10") {
        return TrigCx::Tanh;
    }
    if (lower == "coth" || lower == "11") {
        return TrigCx::Coth;
    }
    if (lower == "asinh" || lower == "12") {
        return TrigCx::Asinh;
    }
    if (lower == "acosh" || lower == "13") {
        return TrigCx::Acosh;
    }
    if (lower == "atanh" || lower == "14") {
        return TrigCx::Atanh;
    }
    if (lower == "acoth" || lower == "15") {
        return TrigCx::Acoth;
    }

    return TrigCx::Sin;
}

void TrigCx_Block::SetDefaultParameters()
{
    m_FunctionType = TrigCx::Sin;
}

