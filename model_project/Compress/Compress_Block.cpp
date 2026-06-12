#include "Compress_Block.h"
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
Compress_Block::Compress_Block(const std::string &name)
    :Block(name)
{

}

bool Compress_Block::Setup()
{
    Block::Setup();
    return true;
}

bool Compress_Block::Run()
{
    std::string inputPort = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    std::vector<double> inputData = ReadInputData<double>(inputPort);
    std::vector<double> outputData(1);
    outputData.reserve(1);
    double inputNorm = inputData[0] / m_Max;
    double inputSgn = inputNorm > 0 ? 1 : -1;

    if (m_CompressionType == Compress::MULaw)
    {
        outputData[0] = m_Max * (inputSgn * std::log(1.0 + m_CompressionK * std::abs(inputNorm))) / (std::log(1.0 + m_CompressionK));
    }

    else if (m_CompressionType == Compress::ALaw)
    {
        if (std::abs(inputNorm) < (1.0 / m_CompressionK))
        {
            outputData[0] = m_Max*(inputSgn * m_CompressionK*std::abs(inputNorm)) / (1.0 + std::log(m_CompressionK));
        }
        else if (std::abs(inputNorm) >= (1.0 / m_CompressionK))
        {
            outputData[0] = m_Max * (inputSgn * (1.0 + std::log(m_CompressionK*std::abs(inputNorm)))) / (1.0 + std::log(m_CompressionK));
        }
    }
    WriteOutputData(outputPort, outputData);
    return true;
}

bool Compress_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_Compress = std::make_unique<Compress>();
    SetDefaultParameters();
    try { m_CompressionType = ConvertStringToSelectedCompressionType(getParameter("CompressionType").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'CompressionType', using default value."); }
    try { m_CompressionK = std::stod(getParameter("CompressionK").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'CompressionK', using default value."); }
    try { m_Max = std::stod(getParameter("Max").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Max', using default value."); }
    SetParameters();
    if(!m_Compress->Setup()) return false;
    AddInputPort("input", m_Compress->input, 1, DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("output", m_Compress->output, 1, DataType::CIRCULAR_BUFFER_DOUBLE);
    return true;
}

void Compress_Block::SetParameters()
{
    if(!m_Compress) return;
    m_Compress->CompressionType = m_CompressionType;
    m_Compress->CompressionK = m_CompressionK;
    m_Compress->Max = m_Max;
}

Compress::SelectedCompressionType Compress_Block::ConvertStringToSelectedCompressionType(const std::string &value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "mulaw" || lower == "0") {
        return Compress::MULaw;
    }
    if (lower == "alaw" || lower == "1") {
        return Compress::ALaw;
    }
    return Compress::MULaw;
}

void Compress_Block::SetDefaultParameters()
{
    m_CompressionType = Compress::MULaw;
    m_CompressionK = 1;
    m_Max = 1;
}
