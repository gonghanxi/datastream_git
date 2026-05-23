#include "Limit_Block.h"
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
Limit_Block::Limit_Block(const std::string &name)
    :Block(name)
{

}

bool Limit_Block::Setup()
{
    Block::Setup();
    return true;
}

bool Limit_Block::Run()
{
    std::vector<double> inputData = ReadInputData<double>(GetInputPortName(0));
    std::vector<double> outputData(1);
    outputData.reserve(1);
    const double PI = std::acos(-1);

    if (LimiterType == Limit::linear)
    {
        if (inputData[0] < Bottom / K)
        {
            outputData[0] = Bottom;
        }
        else if (inputData[0] > Top / K)
        {
            outputData[0] = Top;
        }
        else
        {
            outputData[0] = K * inputData[0];
        }
    }

    if (LimiterType == Limit::atan)
    {
        // SystemVue文档里给的公式是错的，以下面这个公式为准
        outputData[0] = (Top - Bottom) / PI * std::atan(PI*(K*inputData[0] - (Top + Bottom) / 2) / (Top - Bottom)) + (Top + Bottom) / 2.0;
    }
    WriteOutputData(GetOutputPortName(0), outputData);
    return true;
}

bool Limit_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_Limit = std::make_unique<Limit>();
    SetDefaultParameters();
    try { LimiterType = ConvertStringToSelectedLimiterType(getParameter("LimiterType").Value); } catch (...) {}
    try { K = std::stod(getParameter("K").Value); } catch (...) {}
    try { Bottom = std::stod(getParameter("Bottom").Value); } catch (...) {}
    try { Top = std::stod(getParameter("Top").Value); } catch (...) {}
    SetParameters();
    if(!m_Limit->Setup()) return false;
    AddInputPort("input", m_Limit->input, 1, DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("output", m_Limit->output, 1, DataType::CIRCULAR_BUFFER_DOUBLE);
    return true;
}

void Limit_Block::SetParameters()
{
    if(!m_Limit) return;
    m_Limit->K = K;
    m_Limit->Bottom = Bottom;
    m_Limit->LimiterType = LimiterType;
}

Limit::SelectedLimiterType Limit_Block::ConvertStringToSelectedLimiterType(const std::string &value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "linear" || lower == "0") {
        return Limit::linear;
    }
    if (lower == "atan" || lower == "1") {
        return Limit::atan;
    }
    return Limit::linear;
}

void Limit_Block::SetDefaultParameters()
{
    K = 1;
    Bottom = 0;
    Top = 1;
    LimiterType = Limit::linear;
}
