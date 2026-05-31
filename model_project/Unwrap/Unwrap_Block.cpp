#include "Unwrap_Block.h"
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
Unwrap_Block::Unwrap_Block(const std::string &name)
    :Block(name)
{

}

bool Unwrap_Block::Setup()
{
    Block::Setup();
    return true;
}

bool Unwrap_Block::Run()
{
    std::string inputPort = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    auto inputData = ReadInputData<double>(inputPort);
    if (inputData.empty()) {
        return true;
    }
    std::vector<double> outputData(inputData.size());

    const double PI = std::acos(-1);
    for(size_t i = 0; i < inputData.size(); i++) {
        double Period = (m_PhaseType == Unwrap::radians) ? 2.0 * PI : 360.0;
        double PhaseDifference = std::fmod(inputData[i] - m_PrevPhase, Period);
        m_OutPhase += PhaseDifference;
        outputData[i] = m_OutPhase;
        m_PrevPhase = inputData[i];
    }
    WriteOutputData(outputPort, outputData);
    return true;
}

bool Unwrap_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_Unwrap = std::make_unique<Unwrap>();

    SetDefaultParameters();

    try { m_PhaseType = ConvertStringToSelectedPhaseType(getParameter("FunctionType").Value); } catch (...) { }
    try { m_OutPhase = std::stod(getParameter("OutPhase").Value); } catch (...) { }
    try { m_PrevPhase = std::stod(getParameter("PrevPhase").Value); } catch (...) { }

    SetParameters();

    AddInputPort("input", m_Unwrap->input, 1, DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("output", m_Unwrap->output, 1, DataType::CIRCULAR_BUFFER_DOUBLE);

    return true;
}

void Unwrap_Block::SetParameters()
{
    if(!m_Unwrap) return;
    m_Unwrap->OutPhase = m_OutPhase;
    m_Unwrap->PrevPhase = m_PrevPhase;
    m_Unwrap->PhaseType = m_PhaseType;
}

Unwrap::SelectedPhaseType Unwrap_Block::ConvertStringToSelectedPhaseType(const std::string &value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "radians" || lower == "0") {
        return Unwrap::radians;
    }
    if (lower == "degrees" || lower == "1") {
        return Unwrap::degrees;
    }
    return Unwrap::radians;
}

void Unwrap_Block::SetDefaultParameters()
{
    m_PhaseType = Unwrap::radians;
    m_OutPhase = 0;
    m_PrevPhase = 0;
}
