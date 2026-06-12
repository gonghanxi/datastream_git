#include "Expand_Block.h"
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
Expand_Block::Expand_Block(const std::string &name)
    :Block(name)
{

}

bool Expand_Block::Setup()
{
    Block::Setup();
    return true;
}

bool Expand_Block::Run()
{
    std::string inputPort = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    std::vector<double> inputData = ReadInputData<double>(inputPort);
    std::vector<double> outputData(1);
    outputData.reserve(1);
    double inputNorm = inputData[0] / m_Max;
    double inputSgn = inputNorm > 0 ? 1 : -1;

    if (m_CompressionType == Expand::MULaw)
    {
        outputData[0] = m_Max / m_CompressionK * inputSgn*(std::pow((1.0 + m_CompressionK), std::abs(inputNorm)) - 1.0);
    }

    else if (m_CompressionType == Expand::ALaw)
    {
        if (std::abs(inputNorm) < (1.0 / m_CompressionK))
        {
            outputData[0] = m_Max * (1 + std::log(m_CompressionK)) / m_CompressionK * inputSgn;
        }
        else if (std::abs(inputNorm) >= (1.0 / m_CompressionK))
        {
            outputData[0] = m_Max / m_CompressionK * inputSgn*std::exp(std::abs(inputNorm)*(1.0 + std::log(m_CompressionK)) - 1.0);
        }
    }
    WriteOutputData(outputPort, outputData);
    return true;
}

bool Expand_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_Expand = std::make_unique<Expand>();
    SetDefaultParameters();
    try { m_CompressionType = ConvertStringToSelectedCompressionType(getParameter("CompressionType").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'CompressionType', using default value."); }
    try { m_CompressionK = std::stod(getParameter("CompressionK").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'CompressionK', using default value."); }
    try { m_Max = std::stod(getParameter("Max").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Max', using default value."); }
    SetParameters();
    if(!m_Expand->Setup()) return false;
    AddInputPort("input", m_Expand->input, 1, DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("output", m_Expand->output, 1, DataType::CIRCULAR_BUFFER_DOUBLE);
    return true;
}

void Expand_Block::SetParameters()
{
    if(!m_Expand) return;
    m_Expand->CompressionType = m_CompressionType;
    m_Expand->CompressionK = m_CompressionK;
    m_Expand->Max = m_Max;
}

Expand::SelectedCompressionType Expand_Block::ConvertStringToSelectedCompressionType(const std::string &value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "mulaw" || lower == "0") {
        return Expand::MULaw;
    }
    if (lower == "alaw" || lower == "1") {
        return Expand::ALaw;
    }
    return Expand::MULaw;
}

void Expand_Block::SetDefaultParameters()
{
    m_CompressionType = Expand::MULaw;
    m_CompressionK = 1;
    m_Max = 1;
}
