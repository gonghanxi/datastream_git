#include "RADAR_CFAR_M_Block.h"

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

// ============================================================================
// 构造函数
// ============================================================================

RADAR_CFAR_M_Block::RADAR_CFAR_M_Block(const std::string& name)
    : Block(name)
    , m_CFAR_Type(RADAR_CFAR_M::CA)
    , m_CFAR_Dimension(RADAR_CFAR_M::Range)
    , m_CellSize(100)
    , m_ReferenceCell(32)
    , m_GuardCell(4)
    , m_Detector_Type(RADAR_CFAR_M::Square)
    , m_Threshold(RADAR_CFAR_M::ThresholdByPf)
    , m_Pf(1e-4)
    , m_Alpha(1.0)
    , m_Beta(1.0)
    , m_ThresholdFactor(0.0)
    , m_clutterMapInitialized(false)
    , m_inputCount(0)
    , m_outputCount(0)
{
}

// ============================================================================
// 参数设置
// ============================================================================

void RADAR_CFAR_M_Block::SetDefaultParameters()
{
    m_CFAR_Type = RADAR_CFAR_M::CA;
    m_CFAR_Dimension = RADAR_CFAR_M::Range;
    m_CellSize = 100;
    m_ReferenceCell = 32;
    m_GuardCell = 4;
    m_Detector_Type = RADAR_CFAR_M::Square;
    m_Threshold = RADAR_CFAR_M::ThresholdByPf;
    m_Pf = 1e-4;
    m_Alpha = 1.0;
    m_Beta = 1.0;
    m_ThresholdFactor = 0.0;
}

void RADAR_CFAR_M_Block::SetParameters()
{
    if (!m_algo) return;

    m_algo->CFAR_Type = m_CFAR_Type;
    m_algo->CFAR_Dimension = m_CFAR_Dimension;
    m_algo->CellSize = m_CellSize;
    m_algo->ReferenceCell = m_ReferenceCell;
    m_algo->GuardCell = m_GuardCell;
    m_algo->Detector_Type = m_Detector_Type;
    m_algo->Threshold = m_Threshold;
    m_algo->Pf = m_Pf;
    m_algo->Alpha = m_Alpha;
    m_algo->Beta = m_Beta;
    m_algo->ThresholdFactor = m_ThresholdFactor;
}

// ============================================================================
// 参数验证
// ============================================================================

bool RADAR_CFAR_M_Block::ValidateParameters()
{
    if (m_CFAR_Type != RADAR_CFAR_M::ClutterMap) {
        if (m_CellSize <= 0) {
            LOG_ERROR("CellSize must be greater than 0.");
            return false;
        }
        if (m_ReferenceCell <= 0) {
            LOG_ERROR("ReferenceCell must be greater than 0.");
            return false;
        }
        if (m_GuardCell < 0) {
            LOG_ERROR("GuardCell must be greater than or equal to 0.");
            return false;
        }
    }
    return true;
}

// ============================================================================
// UpdateThresholdFactor —— 移植自原算法的 computeThresholdFactor
// ============================================================================

void RADAR_CFAR_M_Block::UpdateThresholdFactor()
{
    if (m_CFAR_Type == RADAR_CFAR_M::ClutterMap) {
        m_ThresholdFactor = m_Alpha;
        return;
    }

    if (m_Threshold == RADAR_CFAR_M::ThresholdByAlpha) {
        m_ThresholdFactor = m_Alpha;
        return;
    }

    // ThresholdByPf: 使用标准 CA-CFAR 近似因子
    int N = 2 * m_ReferenceCell;
    if (N <= 0 || m_Pf <= 0.0 || m_Pf >= 1.0) {
        m_ThresholdFactor = 1.0;
        return;
    }
    m_ThresholdFactor = static_cast<double>(N) * (std::pow(m_Pf, -1.0 / static_cast<double>(N)) - 1.0);
}

// ============================================================================
// Setup
// ============================================================================

bool RADAR_CFAR_M_Block::Setup()
{
    Block::Setup();
    if (!ValidateParameters()) {
        return false;
    }
    UpdateThresholdFactor();

    m_inputBuffer.clear();
    while (!m_outputQueue.empty()) m_outputQueue.pop();
    while (!m_thresholdQueue.empty()) m_thresholdQueue.pop();
    m_inputCount = 0;
    m_outputCount = 0;
    return true;
}

// ============================================================================
// Run —— 双模式分发
// ============================================================================

bool RADAR_CFAR_M_Block::Run()
{
    if (IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

// ============================================================================
// DataStreamRun —— 数据流模式：一次处理整帧矩阵
// ============================================================================

bool RADAR_CFAR_M_Block::DataStreamRun()
{
    auto inputData = ReadInputData<SystemVueModelBuilder::DoubleMatrix>(GetInputPortName(0));
    if (inputData.empty()) {
        return true;
    }

    const SystemVueModelBuilder::DoubleMatrix& inMat = inputData[0];
    const int nRows = static_cast<int>(inMat.NumRows());
    const int nCols = static_cast<int>(inMat.NumColumns());

    SystemVueModelBuilder::DoubleMatrix outMat;
    SystemVueModelBuilder::DoubleMatrix thMat;
    outMat.Resize(nRows, nCols);
    thMat.Resize(nRows, nCols);

    if (nRows <= 0 || nCols <= 0) {
        std::vector<SystemVueModelBuilder::DoubleMatrix> outVec;
        outVec.push_back(outMat);
        WriteOutputData(GetOutputPortName(0), outVec);
        std::vector<SystemVueModelBuilder::DoubleMatrix> thVec;
        thVec.push_back(thMat);
        WriteOutputData(GetOutputPortName(1), thVec);
        return true;
    }

    processMatrix(inMat, outMat, thMat, nRows, nCols);

    std::vector<SystemVueModelBuilder::DoubleMatrix> outVec;
    outVec.push_back(outMat);
    WriteOutputData(GetOutputPortName(0), outVec);

    std::vector<SystemVueModelBuilder::DoubleMatrix> thVec;
    thVec.push_back(thMat);
    WriteOutputData(GetOutputPortName(1), thVec);

    return true;
}

// ============================================================================
// TimeDrivenRun —— 时间驱动模式：逐帧矩阵处理
// ============================================================================

bool RADAR_CFAR_M_Block::TimeDrivenRun()
{
    auto inputData = ReadInputData<SystemVueModelBuilder::DoubleMatrix>(GetInputPortName(0));
    if (inputData.empty()) {
        return true;
    }

    const SystemVueModelBuilder::DoubleMatrix& inMat = inputData[0];
    const int nRows = static_cast<int>(inMat.NumRows());
    const int nCols = static_cast<int>(inMat.NumColumns());

    SystemVueModelBuilder::DoubleMatrix outMat;
    SystemVueModelBuilder::DoubleMatrix thMat;
    outMat.Resize(nRows, nCols);
    thMat.Resize(nRows, nCols);

    if (nRows <= 0 || nCols <= 0) {
        std::vector<SystemVueModelBuilder::DoubleMatrix> outVec;
        outVec.push_back(outMat);
        WriteOutputData(GetOutputPortName(0), outVec);
        std::vector<SystemVueModelBuilder::DoubleMatrix> thVec;
        thVec.push_back(thMat);
        WriteOutputData(GetOutputPortName(1), thVec);
        return true;
    }

    processMatrix(inMat, outMat, thMat, nRows, nCols);

    m_outputQueue.push(outMat);
    m_thresholdQueue.push(thMat);
    m_inputCount++;

    if (!m_outputQueue.empty() && !m_thresholdQueue.empty()) {
        SystemVueModelBuilder::DoubleMatrix outVal = m_outputQueue.front();
        SystemVueModelBuilder::DoubleMatrix thVal = m_thresholdQueue.front();
        m_outputQueue.pop();
        m_thresholdQueue.pop();
        m_outputCount++;

        std::vector<SystemVueModelBuilder::DoubleMatrix> outVec;
        outVec.push_back(outVal);
        WriteOutputData(GetOutputPortName(0), outVec);

        std::vector<SystemVueModelBuilder::DoubleMatrix> thVec;
        thVec.push_back(thVal);
        WriteOutputData(GetOutputPortName(1), thVec);
    }

    return true;
}

// ============================================================================
// processMatrix —— 矩阵 CFAR 核心处理（移植自原算法 Run + 辅助函数）
// ============================================================================

namespace {

// 检波器 —— 移植自 RADAR_CFAR_M::detectorLaw
double detectorLaw(double x, RADAR_CFAR_M::SelectedDetectorType detectorType)
{
    double ax = std::abs(x);
    const double eps = 1e-300;

    switch (detectorType) {
    case RADAR_CFAR_M::Envelope:  return ax;
    case RADAR_CFAR_M::Square:    return ax * ax;
    case RADAR_CFAR_M::LogSquare: return std::log(std::max(ax * ax, eps));
    case RADAR_CFAR_M::Log:       return std::log(std::max(ax, eps));
    default: return ax;
    }
}

// 处理一维序列（CA/SOCA/GOCA）—— 移植自 RADAR_CFAR_M::processOneVector
void processOneVector(
    const std::vector<double>& rawLine,
    std::vector<double>& outLine,
    std::vector<double>& thLine,
    int referenceCell,
    int guardCell,
    RADAR_CFAR_M::SelectedCFARType cfarType,
    double thresholdFactor,
    RADAR_CFAR_M::SelectedDetectorType detectorType)
{
    const int len = static_cast<int>(rawLine.size());
    outLine.assign(len, 0.0);
    thLine.assign(len, 0.0);

    if (len <= 0) return;

    std::vector<double> detLine(len);
    for (int i = 0; i < len; ++i)
        detLine[i] = detectorLaw(rawLine[i], detectorType);

    for (int i = 0; i < len; ++i) {
        double leadingSum = 0.0;
        double laggingSum = 0.0;

        for (int n = 0; n < referenceCell; ++n) {
            int leadingIdx = i + n - guardCell - referenceCell;
            int laggingIdx = i + n + guardCell + 1;

            while (leadingIdx < 0)  leadingIdx += len;
            while (leadingIdx >= len) leadingIdx -= len;
            while (laggingIdx < 0)  laggingIdx += len;
            while (laggingIdx >= len) laggingIdx -= len;

            leadingSum += detLine[leadingIdx];
            laggingSum += detLine[laggingIdx];
        }

        double leadingAvg = leadingSum / static_cast<double>(referenceCell);
        double laggingAvg = laggingSum / static_cast<double>(referenceCell);
        double z = 0.0;

        switch (cfarType) {
        case RADAR_CFAR_M::CA:
            z = (leadingAvg + laggingAvg) / 2.0;
            break;
        case RADAR_CFAR_M::SOCA:
            z = std::min(leadingAvg, laggingAvg);
            break;
        case RADAR_CFAR_M::GOCA:
            z = std::max(leadingAvg, laggingAvg);
            break;
        default:
            z = (leadingAvg + laggingAvg) / 2.0;
            break;
        }

        thLine[i] = thresholdFactor * z;
        outLine[i] = rawLine[i] > thLine[i] ? rawLine[i] : 0.0;
    }
}

// ClutterMap 处理一维序列 —— 移植自 RADAR_CFAR_M::processOneVectorClutterMap
void processOneVectorClutterMap(
    const std::vector<double>& rawLine,
    std::vector<double>& outLine,
    std::vector<double>& thLine,
    std::vector<double>& clutterLine,
    double alpha,
    double beta,
    RADAR_CFAR_M::SelectedDetectorType detectorType)
{
    const int len = static_cast<int>(rawLine.size());
    outLine.assign(len, 0.0);
    thLine.assign(len, 0.0);

    if (len <= 0) return;

    if (static_cast<int>(clutterLine.size()) != len)
        clutterLine.assign(len, 0.0);

    double b = beta;
    if (b < 0.0) b = 0.0;
    if (b > 1.0) b = 1.0;

    for (int i = 0; i < len; ++i) {
        double det = detectorLaw(rawLine[i], detectorType);

        double cNew = (1.0 - b) * clutterLine[i] + b * det;
        clutterLine[i] = cNew;

        thLine[i] = alpha * cNew;
        outLine[i] = det > thLine[i] ? rawLine[i] : 0.0;
    }
}

} // namespace

void RADAR_CFAR_M_Block::processMatrix(
    const SystemVueModelBuilder::DoubleMatrix& inMat,
    SystemVueModelBuilder::DoubleMatrix& outMat,
    SystemVueModelBuilder::DoubleMatrix& thMat,
    int nRows, int nCols)
{
    // ClutterMap 跨帧状态初始化 / 尺寸变化时重置
    if (m_CFAR_Type == RADAR_CFAR_M::ClutterMap) {
        if (!m_clutterMapInitialized
            || static_cast<int>(m_clutterMap.NumRows()) != nRows
            || static_cast<int>(m_clutterMap.NumColumns()) != nCols) {
            m_clutterMap.Resize(nRows, nCols);
            for (int r = 0; r < nRows; ++r)
                for (int c = 0; c < nCols; ++c)
                    m_clutterMap(r, c) = 0.0;
            m_clutterMapInitialized = true;
        }
    } else {
        m_clutterMapInitialized = false;
    }

    if (m_CFAR_Dimension == RADAR_CFAR_M::Range) {
        // Range 方向：每列沿行方向（每个 Doppler bin 沿 Range）做 CFAR
        for (int c = 0; c < nCols; ++c) {
            std::vector<double> rawLine(nRows);
            for (int r = 0; r < nRows; ++r)
                rawLine[r] = inMat(r, c);

            std::vector<double> outLine;
            std::vector<double> thLine;

            if (m_CFAR_Type == RADAR_CFAR_M::ClutterMap) {
                std::vector<double> clutterLine(nRows);
                for (int r = 0; r < nRows; ++r)
                    clutterLine[r] = m_clutterMap(r, c);

                processOneVectorClutterMap(rawLine, outLine, thLine, clutterLine,
                    m_Alpha, m_Beta, m_Detector_Type);

                for (int r = 0; r < nRows; ++r)
                    m_clutterMap(r, c) = clutterLine[r];
            } else {
                processOneVector(rawLine, outLine, thLine,
                    m_ReferenceCell, m_GuardCell, m_CFAR_Type,
                    m_ThresholdFactor, m_Detector_Type);
            }

            for (int r = 0; r < nRows; ++r) {
                outMat(r, c) = outLine[r];
                thMat(r, c) = thLine[r];
            }
        }
    } else {
        // Doppler 方向：每行沿列方向（每个 Range bin 沿 Doppler）做 CFAR
        for (int r = 0; r < nRows; ++r) {
            std::vector<double> rawLine(nCols);
            for (int c = 0; c < nCols; ++c)
                rawLine[c] = inMat(r, c);

            std::vector<double> outLine;
            std::vector<double> thLine;

            if (m_CFAR_Type == RADAR_CFAR_M::ClutterMap) {
                std::vector<double> clutterLine(nCols);
                for (int c = 0; c < nCols; ++c)
                    clutterLine[c] = m_clutterMap(r, c);

                processOneVectorClutterMap(rawLine, outLine, thLine, clutterLine,
                    m_Alpha, m_Beta, m_Detector_Type);

                for (int c = 0; c < nCols; ++c)
                    m_clutterMap(r, c) = clutterLine[c];
            } else {
                processOneVector(rawLine, outLine, thLine,
                    m_ReferenceCell, m_GuardCell, m_CFAR_Type,
                    m_ThresholdFactor, m_Detector_Type);
            }

            for (int c = 0; c < nCols; ++c) {
                outMat(r, c) = outLine[c];
                thMat(r, c) = thLine[c];
            }
        }
    }
}

// ============================================================================
// Initialize
// ============================================================================

bool RADAR_CFAR_M_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_algo = std::make_unique<RADAR_CFAR_M>();

    SetDefaultParameters();

    try { m_CFAR_Type = ConvertStringToCFARType(getParameter("CFAR_Type").Value); } catch (...) {}
    try { m_CFAR_Dimension = ConvertStringToCFARDimension(getParameter("CFAR_Dimension").Value); } catch (...) {}
    try { m_CellSize = std::stoi(getParameter("CellSize").Value); } catch (...) {}
    try { m_ReferenceCell = std::stoi(getParameter("ReferenceCell").Value); } catch (...) {}
    try { m_GuardCell = std::stoi(getParameter("GuardCell").Value); } catch (...) {}
    try { m_Detector_Type = ConvertStringToDetectorType(getParameter("Detector_Type").Value); } catch (...) {}
    try { m_Threshold = ConvertStringToThresholdType(getParameter("Threshold").Value); } catch (...) {}
    try { m_Pf = std::stod(getParameter("Pf").Value); } catch (...) {}
    try { m_Alpha = std::stod(getParameter("Alpha").Value); } catch (...) {}
    try { m_Beta = std::stod(getParameter("Beta").Value); } catch (...) {}

    SetParameters();

    AddInputPort("input", m_algo->input, 1, Block::DataType::MATRIX_DOUBLE);
    AddOutputPort("output", m_algo->output, 1, Block::DataType::MATRIX_DOUBLE);
    AddOutputPort("threshold", m_algo->threshold, 1, Block::DataType::MATRIX_DOUBLE);

    return true;
}

// ============================================================================
// 枚举转换
// ============================================================================

RADAR_CFAR_M::SelectedCFARType RADAR_CFAR_M_Block::ConvertStringToCFARType(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "ca" || lower == "0") {
        return RADAR_CFAR_M::CA;
    }
    if (lower == "soca" || lower == "1") {
        return RADAR_CFAR_M::SOCA;
    }
    if (lower == "goca" || lower == "2") {
        return RADAR_CFAR_M::GOCA;
    }
    if (lower == "clutter map" || lower == "cluttermap" || lower == "3") {
        return RADAR_CFAR_M::ClutterMap;
    }
    return RADAR_CFAR_M::CA;
}

RADAR_CFAR_M::SelectedCFARDimension RADAR_CFAR_M_Block::ConvertStringToCFARDimension(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "range" || lower == "0") {
        return RADAR_CFAR_M::Range;
    }
    if (lower == "doppler" || lower == "1") {
        return RADAR_CFAR_M::Doppler;
    }
    return RADAR_CFAR_M::Range;
}

RADAR_CFAR_M::SelectedDetectorType RADAR_CFAR_M_Block::ConvertStringToDetectorType(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "envelop" || lower == "envelope" || lower == "0") {
        return RADAR_CFAR_M::Envelope;
    }
    if (lower == "square" || lower == "1") {
        return RADAR_CFAR_M::Square;
    }
    if (lower == "logsquare" || lower == "2") {
        return RADAR_CFAR_M::LogSquare;
    }
    if (lower == "log" || lower == "3") {
        return RADAR_CFAR_M::Log;
    }
    return RADAR_CFAR_M::Square;
}

RADAR_CFAR_M::SelectedThresholdType RADAR_CFAR_M_Block::ConvertStringToThresholdType(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "pf" || lower == "0") {
        return RADAR_CFAR_M::ThresholdByPf;
    }
    if (lower == "alpha" || lower == "1") {
        return RADAR_CFAR_M::ThresholdByAlpha;
    }
    return RADAR_CFAR_M::ThresholdByPf;
}
