#include "Trig_Block.h"
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
Trig_Block::Trig_Block(const std::string &name)
    :Block(name)
{

}

bool Trig_Block::Setup()
{
    Block::Setup();
    return true;
}

bool Trig_Block::Run()
{
    if (!CanProcess()) {
        return false;
    }

    std::string inputPort = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    auto inputData = ReadInputData<double>(inputPort);
    if (inputData.empty()) {
        return false;
    }
    std::vector<double> outputData(inputData.size(), 0.0);

    bool bStatus = true;
    const double PI = acos(-1);
    for(size_t i = 0; i < inputData.size(); i++) {
        switch (m_FunctionType)
        {
        case Trig::Sin:
            outputData[i] = std::sin(inputData[i]);
            break;
        case Trig::Cos:
            outputData[i] = std::cos(inputData[i]);
            break;
        case Trig::Tan:
            if (std::cos(inputData[i])==i)
            {
                LOG_ERROR("Tan input is out of domain.(x|x≠kπ+π/2, k∈Z)");
                bStatus = false;
            }
            else
            {
                outputData[i] = std::tan(inputData[i]);
            }
            break;
        case Trig::Cot:
            if (std::sin(inputData[i])==i)
            {
                LOG_ERROR("Cot input is out of domain.(x|x≠kπ, k∈Z)");
                bStatus = false;
            }
            else
            {
                outputData[i] = 1 / std::tan(inputData[i]);
            }
            break;
        case Trig::Asin:
            if (inputData[i] < -1 || inputData[i] > 1)
            {
                LOG_ERROR("Asin input is out of domain.(x|x∈[-1,1])");
                bStatus = false;
            }
            else
            {
                outputData[i] = std::asin(inputData[i]);
            }
            break;
        case Trig::Acos:
            if (inputData[i] < -1 || inputData[i] > 1)
            {
                LOG_ERROR("Acos input is out of domain.(x|x∈[-1,1])");
                bStatus = false;
            }
            else
            {
                outputData[i] = std::acos(inputData[i]);
            }
            break;
        case Trig::Atan:
            outputData[i] = std::atan(inputData[i]);
            break;
        case Trig::Acot:
            outputData[i] = 0.5*PI - std::atan(inputData[i]);
            break;
        case Trig::Sinh:
            outputData[i] = std::sinh(inputData[i]);
            break;
        case Trig::Cosh:
            outputData[i] = std::cosh(inputData[i]);
            break;
        case Trig::Tanh:
            outputData[i] = std::tanh(inputData[i]);
            break;
        case Trig::Coth:
            if (inputData[i] == i)
            {
                LOG_ERROR("Coth input is out of domain. (x|x≠i)");
                bStatus = false;
            }
            else
            {
                outputData[i] = 1.0 / std::tanh(inputData[i]);
            }
            break;
        case Trig::Asinh:
            outputData[i] = std::asinh(inputData[i]);
            break;
        case Trig::Acosh:
            if (inputData[i] < 1)
            {
                LOG_ERROR("Acosh input is out of domain. (x|x∈[1,+∞))");
                bStatus = false;
            }
            else
            {
                outputData[i] = std::acosh(inputData[i]);
            }
            break;
        case Trig::Atanh:
            if (inputData[i] <= -1 || inputData[i] >= 1)
            {
                LOG_ERROR("Atanh input is out of domain. (x|x∈(-1,1))");
                bStatus = false;
            }
            else
            {
                outputData[i] = std::atanh(inputData[i]);
            }
            break;
        case Trig::Acoth:
            if (inputData[i] >= -1 && inputData[i] <= 1)
            {
                LOG_ERROR("Acoth input is out of domain. (x|x∈(-∞,-1)∪(1,+∞))");
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

bool Trig_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_Trig = std::make_unique<Trig>();

    SetDefaultParameters();

    try { m_FunctionType = ConvertStringToSelectedFunctionType(getParameter("FunctionType").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'FunctionType', using default value."); }

    SetParameters();

    AddInputPort("input", m_Trig->input, 1, DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("output", m_Trig->output, 1, DataType::CIRCULAR_BUFFER_DOUBLE);

    return true;
}

void Trig_Block::SetParameters()
{
    if(!m_Trig) return;
    m_Trig->FunctionType = m_FunctionType;
}

Trig::SelectedFunctionType Trig_Block::ConvertStringToSelectedFunctionType(const std::string &value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "sin" || lower == "0") {
        return Trig::Sin;
    }
    if (lower == "cos" || lower == "1") {
        return Trig::Cos;
    }
    if (lower == "tan" || lower == "2") {
        return Trig::Tan;
    }
    if (lower == "cot" || lower == "3") {
        return Trig::Cot;
    }
    if (lower == "asin" || lower == "4") {
        return Trig::Asin;
    }
    if (lower == "acos" || lower == "5") {
        return Trig::Acos;
    }
    if (lower == "atan" || lower == "6") {
        return Trig::Atan;
    }
    if (lower == "acot" || lower == "7") {
        return Trig::Acot;
    }
    if (lower == "sinh" || lower == "8") {
        return Trig::Sinh;
    }
    if (lower == "cosh" || lower == "9") {
        return Trig::Cosh;
    }
    if (lower == "tanh" || lower == "10") {
        return Trig::Tanh;
    }
    if (lower == "coth" || lower == "11") {
        return Trig::Coth;
    }
    if (lower == "asinh" || lower == "12") {
        return Trig::Asinh;
    }
    if (lower == "acosh" || lower == "13") {
        return Trig::Acosh;
    }
    if (lower == "atanh" || lower == "14") {
        return Trig::Atanh;
    }
    if (lower == "acoth" || lower == "15") {
        return Trig::Acoth;
    }

    return Trig::Sin;
}

void Trig_Block::SetDefaultParameters()
{
    m_FunctionType = Trig::Sin;
}
