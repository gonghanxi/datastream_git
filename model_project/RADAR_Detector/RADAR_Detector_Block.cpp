#include "RADAR_Detector_Block.h"

#include <algorithm>
#include <cctype>
#include <cmath>

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

RADAR_Detector_Block::RADAR_Detector_Block(const std::string& name)
    : Block(name)
    , m_detectorType(RADAR_Detector::Square)
    , m_logCoefb(1.0)
    , m_logCoefa(1.0)
{
}

void RADAR_Detector_Block::SetDefaultParamters()
{
    m_detectorType = RADAR_Detector::Square;
    m_logCoefb = 1.0;
    m_logCoefa = 1.0;
}

void RADAR_Detector_Block::SetParameters()
{
    if (!m_radarDetector) {
        return;
    }

    m_radarDetector->DetectorType = m_detectorType;
    m_radarDetector->Log_Coefb = m_logCoefb;
    m_radarDetector->Log_Coefa = m_logCoefa;
}

bool RADAR_Detector_Block::Setup()
{
    Block::Setup();
    return true;
}

bool RADAR_Detector_Block::Run()
{
    if (!CanProcess()) {
        return false;
    }

    std::string inputPort = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    auto inputData = ReadInputData<std::complex<double>>(inputPort);
    if (inputData.empty()) {
        return true;
    }

    std::vector<double> outputData;
    outputData.reserve(inputData.size());

    for (size_t i = 0; i < inputData.size(); ++i)
    {
        const double absx = std::abs(inputData[i]);
        double y = 0.0;

        switch (m_detectorType)
        {
        case RADAR_Detector::Envelop:
            y = absx;
            break;
        case RADAR_Detector::Square:
            y = absx * absx;
            break;
        case RADAR_Detector::LogSquare:
            y = m_logCoefa * std::log(std::abs(m_logCoefb * inputData[i]) * std::abs(m_logCoefb * inputData[i]));
            break;
        case RADAR_Detector::Log:
            y = m_logCoefa * std::log(std::abs(m_logCoefb * inputData[i]));
            break;
        default:
            break;
        }

        outputData.push_back(y);
    }

    WriteOutputData(outputPort, outputData);
    return true;
}

bool RADAR_Detector_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_radarDetector = std::make_unique<RADAR_Detector>();

    AddInputPort("input", m_radarDetector->input, 1, Block::DataType::CIRCULAR_BUFFER_DCOMPLEX);
    AddOutputPort("output", m_radarDetector->output, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);

    SetDefaultParamters();

    try { m_detectorType = ConvertStringToDetectorType(getParameter("DetectorType").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'DetectorType', using default value."); }
    try { m_logCoefb = std::stod(getParameter("Log_Coefb").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Log_Coefb', using default value."); }
    try { m_logCoefa = std::stod(getParameter("Log_Coefa").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Log_Coefa', using default value."); }

    SetParameters();

    return true;
}

RADAR_Detector::SelectedDetectorType RADAR_Detector_Block::ConvertStringToDetectorType(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "envelop" || lower == "0") {
        return RADAR_Detector::Envelop;
    }
    if (lower == "square" || lower == "1") {
        return RADAR_Detector::Square;
    }
    if (lower == "logsquare" || lower == "2") {
        return RADAR_Detector::LogSquare;
    }
    if (lower == "log" || lower == "3") {
        return RADAR_Detector::Log;
    }
    return RADAR_Detector::Square;
}
