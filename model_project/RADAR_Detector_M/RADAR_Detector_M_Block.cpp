#include "RADAR_Detector_M_Block.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <string>

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

RADAR_Detector_M_Block::RADAR_Detector_M_Block(const std::string& name)
    : Block(name)
    , m_Type(RADAR_Detector_M::Square)
    , m_Log_Coefb(1.0)
    , m_Log_Coefa(1.0)
{
}

// ============================================================================
// SetDefaultParameters
// ============================================================================

void RADAR_Detector_M_Block::SetDefaultParameters()
{
    m_Type       = RADAR_Detector_M::Square;
    m_Log_Coefb  = 1.0;
    m_Log_Coefa  = 1.0;
}

// ============================================================================
// SetParameters — 将解析后的参数写入算法对象
// ============================================================================

void RADAR_Detector_M_Block::SetParameters()
{
    if (!m_algo) return;
    m_algo->Type       = m_Type;
    m_algo->Log_Coefb  = m_Log_Coefb;
    m_algo->Log_Coefa  = m_Log_Coefa;
}

// ============================================================================
// ConvertStringToDetectorType
// ============================================================================

RADAR_Detector_M::SelectedDetectorType RADAR_Detector_M_Block::ConvertStringToDetectorType(
    const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "envelop" || lower == "envelope" || lower == "0") {
        return RADAR_Detector_M::Envelop;
    }
    if (lower == "square" || lower == "1") {
        return RADAR_Detector_M::Square;
    }
    if (lower == "log square" || lower == "logsquare" || lower == "2") {
        return RADAR_Detector_M::LogSquare;
    }
    if (lower == "log" || lower == "3") {
        return RADAR_Detector_M::Log;
    }
    return RADAR_Detector_M::Square;
}

// ============================================================================
// Setup
// ============================================================================

bool RADAR_Detector_M_Block::Setup()
{
    Block::Setup();

    m_inputBuffer.clear();
    while (!m_outputQueue.empty()) m_outputQueue.pop();

    SetParameters();
    return true;
}

// ============================================================================
// Run — 双模式分发
// ============================================================================

bool RADAR_Detector_M_Block::Run()
{
    return DataStreamRun();
}

// ============================================================================
// DataStreamRun — 数据流模式：一次处理整帧矩阵
// ============================================================================

bool RADAR_Detector_M_Block::DataStreamRun()
{
    auto inputData = ReadInputData<SystemVueModelBuilder::DComplexMatrix>(GetInputPortName(0));
    if (inputData.empty()) {
        return true;
    }

    const SystemVueModelBuilder::DComplexMatrix& inMat = inputData[0];
    const size_t nRows = inMat.NumRows();
    const size_t nCols = inMat.NumColumns();

    SystemVueModelBuilder::DoubleMatrix outMat;
    outMat.Resize(nRows, nCols);

    for (size_t r = 0; r < nRows; ++r) {
        for (size_t c = 0; c < nCols; ++c) {
            const std::complex<double>& x = inMat(r, c);

            switch (m_Type) {
            case RADAR_Detector_M::Envelop:
                outMat(r, c) = std::abs(x);
                break;
            case RADAR_Detector_M::Square:
                outMat(r, c) = std::abs(x) * std::abs(x);
                break;
            case RADAR_Detector_M::LogSquare:
                outMat(r, c) = m_Log_Coefa * std::log(
                    std::abs(m_Log_Coefb * x) * std::abs(m_Log_Coefb * x));
                break;
            case RADAR_Detector_M::Log:
                outMat(r, c) = m_Log_Coefa * std::log(std::abs(m_Log_Coefb * x));
                break;
            default:
                outMat(r, c) = 0.0;
                break;
            }
        }
    }

    std::vector<SystemVueModelBuilder::DoubleMatrix> outVec;
    outVec.push_back(outMat);
    WriteOutputData(GetOutputPortName(0), outVec);

    return true;
}

// ============================================================================
// Initialize
// ============================================================================

bool RADAR_Detector_M_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_algo = std::make_unique<RADAR_Detector_M>();

    SetDefaultParameters();

    try { m_Type       = ConvertStringToDetectorType(getParameter("Type").Value); } catch (...) {}
    try { m_Log_Coefb  = std::stod(getParameter("Log_Coefb").Value); }             catch (...) {}
    try { m_Log_Coefa  = std::stod(getParameter("Log_Coefa").Value); }             catch (...) {}

    SetParameters();

    AddInputPort("input",  m_algo->input,  1, Block::DataType::MATRIX_DCOMPLEX);
    AddOutputPort("output", m_algo->output, 1, Block::DataType::MATRIX_DOUBLE);

    return true;
}
