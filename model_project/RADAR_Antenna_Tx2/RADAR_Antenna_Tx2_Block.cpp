#include "RADAR_Antenna_Tx2_Block.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <complex>
#include <sstream>
#include <vector>

#ifndef M_PI
#define M_PI 3.141592653589793238462643383279502884
#endif

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
// 参数解析
// ============================================================================

bool RADAR_Antenna_Tx2_Block::parseArrayString(const std::string& arrayStr, std::vector<double>& outArray)
{
    outArray.clear();

    std::string str = arrayStr;
    size_t start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return false;
    size_t end = str.find_last_not_of(" \t\n\r");
    str = str.substr(start, end - start + 1);

    if (str.empty() || str.front() != '[' || str.back() != ']') {
        return false;
    }

    std::string content = str.substr(1, str.length() - 2);

    start = content.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) {
        return true;
    }
    end = content.find_last_not_of(" \t\n\r");
    content = content.substr(start, end - start + 1);

    std::stringstream ss(content);
    std::string item;

    while (std::getline(ss, item, ',')) {
        start = item.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) continue;
        end = item.find_last_not_of(" \t\n\r");
        item = item.substr(start, end - start + 1);

        if (!item.empty()) {
            try {
                double value = std::stod(item);
                outArray.push_back(value);
            } catch (const std::exception& e) {
                LOG_ERROR("Failed to parse array element: ", item, " - ", e.what());
                return false;
            }
        }
    }

    return true;
}

// ============================================================================
// 生命周期
// ============================================================================

RADAR_Antenna_Tx2_Block::RADAR_Antenna_Tx2_Block(const std::string& name)
    : Block(name)
{
}

void RADAR_Antenna_Tx2_Block::SetDefaultParamters()
{
    m_RadarWorkMode = SelectedRadarWorkMode::Tracking;
    m_Pattern = SelectedPattern::Uniform;
    m_Factor1 = 0.0;
    m_Factor2 = 0.0;
    m_AntennaPatternArray = nullptr;
    m_AntennaPatternArray_Size = 0;
    m_Sidelobe_Levels = -20.0;
    m_nBar = 2;
    m_AntennaHeight = 5.0;
    m_AntennaWidth = 5.0;
    m_AntennaScanPattern = SelectedAntennaScanPattern::CircularScan;
    m_ScanRate = 15.0;
    m_ElevationAngle = 0.0;
    m_SectorScanStartAngle = 0.0;
    m_SectorScanEndAngle = 0.0;
    m_FlybackTime = 0.0;
    m_RetraceTime = 0.0;
    m_NumberOfRasterBars = 0;
    m_RasterBarWidth = 5.0;
    m_TargetAzimuthAngle = nullptr;
    m_TargetAzimuthAngle_Size = 0;
    m_TargetElevationAngle = nullptr;
    m_TargetElevationAngle_Size = 0;
    m_BeamAzimuthAngle = 0.0;
    m_BeamElevationAngle = 0.0;
    m_AntennaEfficiency = 1.0;
}

void RADAR_Antenna_Tx2_Block::SetParameters()
{
    m_AntennaPatternArray = primdata.data();
    m_AntennaPatternArray_Size = static_cast<int>(primdata.size());
    if (!m_ant) { return; }
    m_ant->RadarWorkMode = m_RadarWorkMode;
    m_ant->Pattern = m_Pattern;
    m_ant->Factor1 = m_Factor1;
    m_ant->Factor2 = m_Factor2;
    m_ant->AntennaPatternArray = m_AntennaPatternArray;
    m_ant->AntennaPatternArray_Size = m_AntennaPatternArray_Size;
    m_ant->Sidelobe_Levels = m_Sidelobe_Levels;
    m_ant->nBar = m_nBar;
    m_ant->AntennaHeight = m_AntennaHeight;
    m_ant->AntennaWidth = m_AntennaWidth;
    m_ant->AntennaScanPattern = m_AntennaScanPattern;
    m_ant->ScanRate = m_ScanRate;
    m_ant->ElevationAngle = m_ElevationAngle;
    m_ant->SectorScanStartAngle = m_SectorScanStartAngle;
    m_ant->SectorScanEndAngle = m_SectorScanEndAngle;
    m_ant->FlybackTime = m_FlybackTime;
    m_ant->RetraceTime = m_RetraceTime;
    m_ant->NumberOfRasterBars = m_NumberOfRasterBars;
    m_ant->RasterBarWidth = m_RasterBarWidth;
    m_ant->TargetAzimuthAngle = m_TargetAzimuthAngle;
    m_ant->TargetAzimuthAngle_Size = m_TargetAzimuthAngle_Size;
    m_ant->TargetElevationAngle = m_TargetElevationAngle;
    m_ant->TargetElevationAngle_Size = m_TargetElevationAngle_Size;
    m_ant->BeamAzimuthAngle = m_BeamAzimuthAngle;
    m_ant->BeamElevationAngle = m_BeamElevationAngle;
    m_ant->AntennaEfficiency = m_AntennaEfficiency;
}

bool RADAR_Antenna_Tx2_Block::Setup()
{
    Block::Setup();

    m_inputBuffer.clear();
    while (!m_outputQueue.empty()) m_outputQueue.pop();

    SetParameters();
    return true;
}

bool RADAR_Antenna_Tx2_Block::Run()
{
    if (IsVariableStepMode()) {
        return TimeDrivenRun();
    }
    return DataStreamRun();
}

bool RADAR_Antenna_Tx2_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_ant = std::make_unique<RADAR_Antenna_Tx2>();
    SetDefaultParamters();

    try { m_RadarWorkMode = ConvertStringToRadarWorkMode(getParameter("RadarWorkMode").Value); } catch (...) {}
    try { m_Pattern = ConvertStringToPattern(getParameter("Pattern").Value); } catch (...) {}
    try { m_Factor1 = std::stod(getParameter("Factor1").Value); } catch (...) {}
    try { m_Factor2 = std::stod(getParameter("Factor2").Value); } catch (...) {}
    try {
        std::string PrimString = getParameter("AntennaPatternArray").Value;
        parseArrayString(PrimString, primdata);
    } catch(...) {}
    try { m_Sidelobe_Levels = std::stod(getParameter("Sidelobe_Levels").Value); } catch (...) {}
    try { m_nBar = std::stoi(getParameter("nBar").Value); } catch (...) {}
    try { m_AntennaHeight = std::stod(getParameter("AntennaHeight").Value); } catch (...) {}
    try { m_AntennaWidth = std::stod(getParameter("AntennaWidth").Value); } catch (...) {}
    try { m_AntennaScanPattern = ConvertStringToAntennaScanPattern(getParameter("AntennaScanPattern").Value); } catch (...) {}
    try { m_ScanRate = std::stod(getParameter("ScanRate").Value); } catch (...) {}
    try { m_ElevationAngle = std::stod(getParameter("ElevationAngle").Value); } catch (...) {}
    try { m_SectorScanStartAngle = std::stod(getParameter("SectorScanStartAngle").Value); } catch (...) {}
    try { m_SectorScanEndAngle = std::stod(getParameter("SectorScanEndAngle").Value); } catch (...) {}
    try { m_FlybackTime = std::stod(getParameter("FlybackTime").Value); } catch (...) {}
    try { m_RetraceTime = std::stod(getParameter("RetraceTime").Value); } catch (...) {}
    try { m_NumberOfRasterBars = std::stoi(getParameter("NumberOfRasterBars").Value); } catch (...) {}
    try { m_RasterBarWidth = std::stod(getParameter("RasterBarWidth").Value); } catch (...) {}
    try { m_BeamAzimuthAngle = std::stod(getParameter("BeamAzimuthAngle").Value); } catch (...) {}
    try { m_BeamElevationAngle = std::stod(getParameter("BeamElevationAngle").Value); } catch (...) {}
    try { m_AntennaEfficiency = std::stod(getParameter("AntennaEfficiency").Value); } catch (...) {}

    SetParameters();

    AddInputPort("TargetAzimuth",   m_ant->TargetAzimuth,   1, Block::DataType::DOUBLE_BUS);
    AddInputPort("TargetElevation", m_ant->TargetElevation, 1, Block::DataType::DOUBLE_BUS);
    AddInputPort("BeamAzimuth",     m_ant->BeamAzimuth,     1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddInputPort("BeamElevation",   m_ant->BeamElevation,   1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddInputPort("input",           m_ant->input,           1, Block::DataType::ENVELOPE_SIGNAL);
    AddOutputPort("output",         m_ant->output,          1, Block::DataType::ENVELOPE_BUS);

    return true;
}

// ============================================================================
// DataStreamRun — 定步长模式
// ============================================================================

bool RADAR_Antenna_Tx2_Block::DataStreamRun()
{
    auto inputData = ReadInputData<EnvelopeSignal>(GetInputPortName(4));
    if (inputData.empty()) { return false; }

    const EnvelopeSignal xin = inputData[0];

    auto azData = ReadInputData<double>(GetInputPortName(0));
    auto elData = ReadInputData<double>(GetInputPortName(1));

    int targetCount = 1;
    if (!azData.empty() || !elData.empty()) {
        targetCount = static_cast<int>(std::max(azData.size(), elData.size()));
    } else {
        targetCount = std::max(m_TargetAzimuthAngle_Size, m_TargetElevationAngle_Size);
        if (targetCount <= 0) { targetCount = 1; }
    }

    if (targetCount <= 0) { return true; }

    double fcHz = 0.0;
    {
        auto* reader = GetInputPort(GetInputPortName(4));
        if (reader && reader->hasCharacterizationFrequency()) {
            fcHz = reader->getCharacterizationFrequency();
        }
    }
    if (fcHz <= 0.0) { fcHz = 1.0e9; }

    auto bAzData = ReadInputData<double>(GetInputPortName(2));
    auto bElData = ReadInputData<double>(GetInputPortName(3));
    bool hasBeamAzPort = !bAzData.empty();
    bool hasBeamElPort = !bElData.empty();

    double beamAzRad = 0.0, beamElRad = 0.0;
    double timeNow = 0.0;

    if (hasBeamAzPort) { beamAzRad = bAzData[0]; }
    if (hasBeamElPort) { beamElRad = bElData[0]; }
    if (!hasBeamAzPort || !hasBeamElPort) {
        double azTmp = 0.0, elTmp = 0.0;
        getBeamAngle(timeNow, azTmp, elTmp);
        if (!hasBeamAzPort) { beamAzRad = azTmp; }
        if (!hasBeamElPort) { beamElRad = elTmp; }
    }

    std::vector<EnvelopeSignal> outSignals(targetCount);

    for (int ch = 0; ch < targetCount; ++ch) {
        double targetAzRad = 0.0, targetElRad = 0.0;

        if (!azData.empty() && ch < static_cast<int>(azData.size())) {
            targetAzRad = azData[ch];
        } else {
            targetAzRad = deg2rad(getArrayValue(m_TargetAzimuthAngle, m_TargetAzimuthAngle_Size, ch, 0.0));
        }

        if (!elData.empty() && ch < static_cast<int>(elData.size())) {
            targetElRad = elData[ch];
        } else {
            targetElRad = deg2rad(getArrayValue(m_TargetElevationAngle, m_TargetElevationAngle_Size, ch, 0.0));
        }

        const double ampGain = calcAntennaAmplitudeGain(targetAzRad, targetElRad, beamAzRad, beamElRad, fcHz);

        outSignals[ch] = EnvelopeSignal(xin.complex() * ampGain);
    }

    WriteOutputData(GetOutputPortName(0), outSignals);
    return true;
}

// ============================================================================
// TimeDrivenRun — 变步长模式
// ============================================================================

bool RADAR_Antenna_Tx2_Block::TimeDrivenRun()
{
    // ① 累积输入
    {
        auto inputData = ReadInputData<EnvelopeSignal>(GetInputPortName(4));
        if (inputData.empty()) {
            goto output_label;
        }

        double fcHz = 0.0;
        {
            auto* reader = GetInputPort(GetInputPortName(4));
            if (reader && reader->hasCharacterizationFrequency()) {
                fcHz = reader->getCharacterizationFrequency();
            }
        }
        if (fcHz <= 0.0) { fcHz = 1.0e9; }

        auto azData  = ReadInputData<double>(GetInputPortName(0));
        auto elData  = ReadInputData<double>(GetInputPortName(1));
        auto bAzData = ReadInputData<double>(GetInputPortName(2));
        auto bElData = ReadInputData<double>(GetInputPortName(3));

        InputSnapshot in;
        in.targetAz.assign(azData.begin(), azData.end());
        in.targetEl.assign(elData.begin(), elData.end());
        in.beamAz = bAzData.empty() ? 0.0 : bAzData[0];
        in.beamEl = bElData.empty() ? 0.0 : bElData[0];
        in.hasBeamAzPort = !bAzData.empty();
        in.hasBeamElPort = !bElData.empty();
        in.inputSignal = inputData[0];
        in.fcHz = fcHz;
        m_inputBuffer.push_back(in);
    }

    // ② 处理所有累积输入 → 入队
    while (!m_inputBuffer.empty()) {
        InputSnapshot in = m_inputBuffer.front();
        m_inputBuffer.erase(m_inputBuffer.begin());

        int targetCount = 1;
        if (!in.targetAz.empty() || !in.targetEl.empty()) {
            targetCount = static_cast<int>(std::max(in.targetAz.size(), in.targetEl.size()));
        } else {
            targetCount = std::max(m_TargetAzimuthAngle_Size, m_TargetElevationAngle_Size);
            if (targetCount <= 0) { targetCount = 1; }
        }

        if (targetCount <= 0) { continue; }

        double beamAzRad = 0.0, beamElRad = 0.0;
        double timeNow = 0.0;

        if (in.hasBeamAzPort) { beamAzRad = in.beamAz; }
        if (in.hasBeamElPort) { beamElRad = in.beamEl; }
        if (!in.hasBeamAzPort || !in.hasBeamElPort) {
            double azTmp = 0.0, elTmp = 0.0;
            getBeamAngle(timeNow, azTmp, elTmp);
            if (!in.hasBeamAzPort) { beamAzRad = azTmp; }
            if (!in.hasBeamElPort) { beamElRad = elTmp; }
        }

        std::vector<EnvelopeSignal> outSignals(targetCount);
        std::complex<double> xval = in.inputSignal.complex();

        for (int ch = 0; ch < targetCount; ++ch) {
            double targetAzRad = 0.0, targetElRad = 0.0;

            if (!in.targetAz.empty() && ch < static_cast<int>(in.targetAz.size())) {
                targetAzRad = in.targetAz[ch];
            } else {
                targetAzRad = deg2rad(getArrayValue(m_TargetAzimuthAngle, m_TargetAzimuthAngle_Size, ch, 0.0));
            }

            if (!in.targetEl.empty() && ch < static_cast<int>(in.targetEl.size())) {
                targetElRad = in.targetEl[ch];
            } else {
                targetElRad = deg2rad(getArrayValue(m_TargetElevationAngle, m_TargetElevationAngle_Size, ch, 0.0));
            }

            const double ampGain = calcAntennaAmplitudeGain(targetAzRad, targetElRad, beamAzRad, beamElRad, in.fcHz);

            outSignals[ch] = EnvelopeSignal(xval * ampGain);
        }

        OutputFrame outFrame;
        outFrame.outputSignals = std::move(outSignals);
        m_outputQueue.push(std::move(outFrame));
    }

    // ③ 出队写入
output_label:
    if (!m_outputQueue.empty()) {
        OutputFrame outFrame = std::move(m_outputQueue.front());
        m_outputQueue.pop();
        WriteOutputData(GetOutputPortName(0), outFrame.outputSignals);
    }

    return true;
}

// ============================================================================
// ConvertStringTo 系列
// ============================================================================

RADAR_Antenna_Tx2_Block::SelectedRadarWorkMode
RADAR_Antenna_Tx2_Block::ConvertStringToRadarWorkMode(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "tracking" || lower == "0") return SelectedRadarWorkMode::Tracking;
    if (lower == "search" || lower == "1")   return SelectedRadarWorkMode::Search;
    return SelectedRadarWorkMode::Tracking;
}

RADAR_Antenna_Tx2_Block::SelectedPattern
RADAR_Antenna_Tx2_Block::ConvertStringToPattern(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "userdefinedpattern" || lower == "0")     return SelectedPattern::UserDefinedPattern;
    if (lower == "uniform" || lower == "1")                 return SelectedPattern::Uniform;
    if (lower == "cosine" || lower == "2")                  return SelectedPattern::Cosine;
    if (lower == "parabolic" || lower == "3")               return SelectedPattern::Parabolic;
    if (lower == "triangle" || lower == "4")                return SelectedPattern::Triangle;
    if (lower == "circular" || lower == "5")                return SelectedPattern::Circular;
    if (lower == "cosinesquaredpedestal" || lower == "6")   return SelectedPattern::CosineSquaredPedestal;
    if (lower == "taylor" || lower == "7")                  return SelectedPattern::Taylor;
    return SelectedPattern::Uniform;
}

RADAR_Antenna_Tx2_Block::SelectedAntennaScanPattern
RADAR_Antenna_Tx2_Block::ConvertStringToAntennaScanPattern(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "circular" || lower == "0")                 return SelectedAntennaScanPattern::CircularScan;
    if (lower == "bidirectional sector" || lower == "1")     return SelectedAntennaScanPattern::BidirectionalSector;
    if (lower == "unidirectional sector" || lower == "2")    return SelectedAntennaScanPattern::UnidirectionalSector;
    if (lower == "bidirectional raster" || lower == "3")     return SelectedAntennaScanPattern::BidirectionalRaster;
    if (lower == "unidirectional raster" || lower == "4")    return SelectedAntennaScanPattern::UnidirectionalRaster;
    return SelectedAntennaScanPattern::CircularScan;
}

// ============================================================================
// 基础数学工具
// ============================================================================

double RADAR_Antenna_Tx2_Block::deg2rad(double x) { return x * M_PI / 180.0; }
double RADAR_Antenna_Tx2_Block::rad2deg(double x) { return x * 180.0 / M_PI; }

double RADAR_Antenna_Tx2_Block::normalizeRad(double x)
{
    while (x > M_PI)  { x -= 2.0 * M_PI; }
    while (x < -M_PI) { x += 2.0 * M_PI; }
    return x;
}

double RADAR_Antenna_Tx2_Block::wrap360(double x)
{
    double y = std::fmod(x, 360.0);
    if (y < 0.0) { y += 360.0; }
    return y;
}

double RADAR_Antenna_Tx2_Block::sinc(double x)
{
    if (std::fabs(x) < 1.0e-12) return 1.0;
    return std::sin(x) / x;
}

double RADAR_Antenna_Tx2_Block::besselI0(double x)
{
    double sum = 1.0, term = 1.0;
    const double xx = x * x / 4.0;
    for (int k = 1; k <= 20; ++k) {
        term *= xx / static_cast<double>(k * k);
        sum += term;
    }
    return sum;
}

double RADAR_Antenna_Tx2_Block::getArrayValue(const double* data, int size, int index, double defaultValue) const
{
    if (data == nullptr || size <= 0) return defaultValue;
    if (index < 0)  return data[0];
    if (index < size) return data[index];
    return data[size - 1];
}

// ============================================================================
// Beam 角度计算
// ============================================================================

void RADAR_Antenna_Tx2_Block::getBeamAngle(double timeNow, double& beamAzRad, double& beamElRad)
{
    beamAzRad = 0.0;
    beamElRad = 0.0;

    if (m_RadarWorkMode == SelectedRadarWorkMode::Search) {
        if (m_AntennaScanPattern == SelectedAntennaScanPattern::CircularScan) {
            beamAzRad = deg2rad(getCircularScanAzimuth(timeNow));
        } else if (m_AntennaScanPattern == SelectedAntennaScanPattern::BidirectionalSector) {
            beamAzRad = deg2rad(getSectorScanAzimuth(timeNow, true));
        } else if (m_AntennaScanPattern == SelectedAntennaScanPattern::UnidirectionalSector) {
            beamAzRad = deg2rad(getSectorScanAzimuth(timeNow, false));
        } else if (m_AntennaScanPattern == SelectedAntennaScanPattern::BidirectionalRaster) {
            double azDeg = 0.0, elDeg = 0.0;
            getRasterScanAngle(timeNow, true, azDeg, elDeg);
            beamAzRad = deg2rad(azDeg);
        } else if (m_AntennaScanPattern == SelectedAntennaScanPattern::UnidirectionalRaster) {
            double azDeg = 0.0, elDeg = 0.0;
            getRasterScanAngle(timeNow, false, azDeg, elDeg);
            beamAzRad = deg2rad(azDeg);
        } else {
            beamAzRad = deg2rad(m_BeamAzimuthAngle);
        }
    } else {
        beamAzRad = deg2rad(m_BeamAzimuthAngle);
    }

    if (m_RadarWorkMode == SelectedRadarWorkMode::Search &&
        (m_AntennaScanPattern == SelectedAntennaScanPattern::BidirectionalRaster ||
         m_AntennaScanPattern == SelectedAntennaScanPattern::UnidirectionalRaster)) {
        double azDeg = 0.0, elDeg = 0.0;
        getRasterScanAngle(timeNow,
            m_AntennaScanPattern == SelectedAntennaScanPattern::BidirectionalRaster,
            azDeg, elDeg);
        beamElRad = deg2rad(elDeg);
    } else if (m_RadarWorkMode == SelectedRadarWorkMode::Search) {
        beamElRad = deg2rad(m_ElevationAngle);
    } else {
        beamElRad = deg2rad(m_BeamElevationAngle);
    }
}

double RADAR_Antenna_Tx2_Block::getCircularScanAzimuth(double timeNow) const
{
    const double rateDegPerSec = m_ScanRate * 6.0;
    if (rateDegPerSec == 0.0) { return 0.0; }
    return wrap360(rateDegPerSec * timeNow);
}

double RADAR_Antenna_Tx2_Block::getSectorScanAzimuth(double timeNow, bool bidirectional) const
{
    const double startDeg = m_SectorScanStartAngle;
    const double endDeg   = m_SectorScanEndAngle;
    double width = endDeg - startDeg;

    if (std::fabs(width) < 1.0e-15) { return startDeg; }

    const double dir = (width >= 0.0) ? 1.0 : -1.0;
    width = std::fabs(width);

    const double rateDegPerSec = std::fabs(m_ScanRate * 6.0);
    if (rateDegPerSec <= 0.0) { return startDeg; }

    const double forwardTime = width / rateDegPerSec;

    if (bidirectional) {
        const double period = 2.0 * forwardTime;
        if (period <= 0.0) { return startDeg; }
        double t = std::fmod(timeNow, period);
        if (t < 0.0) { t += period; }
        if (t <= forwardTime) {
            return startDeg + dir * rateDegPerSec * t;
        } else {
            return endDeg - dir * rateDegPerSec * (t - forwardTime);
        }
    } else {
        const double fb = std::max(0.0, m_FlybackTime);
        const double period = forwardTime + fb;
        if (period <= 0.0) { return startDeg; }
        double t = std::fmod(timeNow, period);
        if (t < 0.0) { t += period; }
        if (t <= forwardTime) {
            return startDeg + dir * rateDegPerSec * t;
        }
        if (fb > 0.0) {
            const double k = (t - forwardTime) / fb;
            return endDeg + (startDeg - endDeg) * k;
        }
        return startDeg;
    }
}

void RADAR_Antenna_Tx2_Block::getRasterScanAngle(double timeNow, bool bidirectional,
                                                  double& azDeg, double& elDeg) const
{
    const int barCount = std::max(1, m_NumberOfRasterBars + 1);

    const double startDeg = m_SectorScanStartAngle;
    const double endDeg   = m_SectorScanEndAngle;

    double width = endDeg - startDeg;
    if (std::fabs(width) < 1.0e-15) {
        azDeg = startDeg;
        elDeg = m_ElevationAngle;
        return;
    }

    const double dir = (width >= 0.0) ? 1.0 : -1.0;
    width = std::fabs(width);

    const double rateDegPerSec = std::fabs(m_ScanRate * 6.0);
    if (rateDegPerSec <= 0.0) {
        azDeg = startDeg;
        elDeg = m_ElevationAngle;
        return;
    }

    const double scanTime = width / rateDegPerSec;
    const double fb = std::max(0.0, m_FlybackTime);
    const double rt = std::max(0.0, m_RetraceTime);

    double rowTime = 0.0;
    if (bidirectional) {
        rowTime = scanTime;
    } else {
        rowTime = scanTime + fb;
    }

    const double activeTime = rowTime * barCount;
    const double period = activeTime + rt;

    if (period <= 0.0) {
        azDeg = startDeg;
        elDeg = m_ElevationAngle;
        return;
    }

    double t = std::fmod(timeNow, period);
    if (t < 0.0) { t += period; }

    if (t >= activeTime) {
        azDeg = startDeg;
        elDeg = m_ElevationAngle;
        return;
    }

    int row = static_cast<int>(t / rowTime);
    if (row >= barCount) { row = barCount - 1; }

    double rowLocal = t - row * rowTime;
    elDeg = m_ElevationAngle + row * m_RasterBarWidth;

    if (bidirectional) {
        const bool reverse = (row % 2) != 0;
        if (!reverse) {
            azDeg = startDeg + dir * rateDegPerSec * rowLocal;
        } else {
            azDeg = endDeg - dir * rateDegPerSec * rowLocal;
        }
    } else {
        if (rowLocal <= scanTime) {
            azDeg = startDeg + dir * rateDegPerSec * rowLocal;
        } else {
            if (fb > 0.0) {
                const double k = (rowLocal - scanTime) / fb;
                azDeg = endDeg + (startDeg - endDeg) * k;
            } else {
                azDeg = startDeg;
            }
        }
    }
}

// ============================================================================
// 天线增益 — Tx2 核心差异：AntennaEfficiency = sqrt(η) 乘子
// ============================================================================

double RADAR_Antenna_Tx2_Block::angularSeparation(double az1, double el1,
                                                   double az2, double el2) const
{
    const double x1 = std::cos(el1) * std::cos(az1);
    const double y1 = std::cos(el1) * std::sin(az1);
    const double z1 = std::sin(el1);
    const double x2 = std::cos(el2) * std::cos(az2);
    const double y2 = std::cos(el2) * std::sin(az2);
    const double z2 = std::sin(el2);
    double dot = x1 * x2 + y1 * y2 + z1 * z2;
    if (dot > 1.0) { dot = 1.0; }
    if (dot < -1.0) { dot = -1.0; }
    return std::acos(dot);
}

double RADAR_Antenna_Tx2_Block::calcAntennaAmplitudeGain(double targetAzRad, double targetElRad,
                                                          double beamAzRad, double beamElRad,
                                                          double fcHz) const
{
    const double sep = angularSeparation(targetAzRad, targetElRad, beamAzRad, beamElRad);
    if (sep > 0.5 * M_PI) { return 0.0; }

    // Tx2 新增：天线总效率 sqrt(η)
    double eta = m_AntennaEfficiency;
    if (eta < 0.0) { eta = 0.0; }
    const double efficiencyAmpGain = std::sqrt(eta);

    if (m_Pattern == SelectedPattern::UserDefinedPattern) {
        return efficiencyAmpGain * calcUserPatternGain(targetAzRad, targetElRad, beamAzRad, beamElRad);
    }

    const double c = 3.0e8;
    if (fcHz <= 0.0) { return 0.0; }
    const double lambda = c / fcHz;
    if (lambda <= 0.0) { return 0.0; }

    if (m_AntennaHeight <= 0.0 || m_AntennaWidth <= 0.0) { return 0.0; }

    const double apertureArea = m_AntennaHeight * m_AntennaWidth;
    const double apertureAmpGain = efficiencyAmpGain * std::sqrt(4.0 * M_PI * apertureArea) / lambda;

    const double dAz = normalizeRad(targetAzRad - beamAzRad);
    const double dEl = normalizeRad(targetElRad - beamElRad);

    const double patternFactor = calcAnalyticPatternFactor(dAz, dEl, lambda);

    return apertureAmpGain * patternFactor;
}

double RADAR_Antenna_Tx2_Block::calcUserPatternGain(double targetAzRad, double targetElRad,
                                                     double beamAzRad, double beamElRad) const
{
    if (m_AntennaPatternArray == nullptr || m_AntennaPatternArray_Size <= 0) {
        return 1.0;
    }

    double dAzDeg = rad2deg(normalizeRad(targetAzRad - beamAzRad));
    double dElDeg = rad2deg(normalizeRad(targetElRad - beamElRad));

    int elIndex = static_cast<int>(std::floor(dElDeg + 90.0));
    int azIndex = static_cast<int>(std::floor(dAzDeg + 180.0));

    if (elIndex < 0)  { elIndex = 0; }
    if (elIndex > 179) { elIndex = 179; }

    while (azIndex < 0)   { azIndex += 360; }
    while (azIndex >= 360) { azIndex -= 360; }

    const int index = elIndex * 360 + azIndex;

    double gainDb = 0.0;
    if (index >= 0 && index < m_AntennaPatternArray_Size) {
        gainDb = m_AntennaPatternArray[index];
    } else {
        gainDb = m_AntennaPatternArray[m_AntennaPatternArray_Size - 1];
    }

    return std::pow(10.0, gainDb / 20.0);
}

double RADAR_Antenna_Tx2_Block::calcAnalyticPatternFactor(double dAzRad, double dElRad,
                                                           double lambda) const
{
    if (lambda <= 0.0) { return 0.0; }

    const double u = std::sin(dAzRad);
    const double v = std::sin(dElRad);

    const double xAz = M_PI * m_AntennaWidth / lambda * u;
    const double xEl = M_PI * m_AntennaHeight / lambda * v;

    double base = std::fabs(sinc(xAz) * sinc(xEl));
    if (base < 0.0) { base = 0.0; }

    switch (m_Pattern) {
    case SelectedPattern::Uniform:
        return base;

    case SelectedPattern::Cosine: {
        const double n = std::max(0.0, m_Factor1);
        const double w = std::pow(std::max(0.0, std::cos(0.5 * dAzRad)), n + 1.0) *
                         std::pow(std::max(0.0, std::cos(0.5 * dElRad)), n + 1.0);
        return base * w;
    }

    case SelectedPattern::Parabolic: {
        const double delta = m_Factor1;
        const double r2 = u * u + v * v;
        double w = 1.0 - delta * r2;
        if (w < 0.0) { w = 0.0; }
        return base * w;
    }

    case SelectedPattern::Triangle: {
        const double waz = std::max(0.0, 1.0 - std::fabs(dAzRad) / (0.5 * M_PI));
        const double wel = std::max(0.0, 1.0 - std::fabs(dElRad) / (0.5 * M_PI));
        return base * waz * wel;
    }

    case SelectedPattern::Circular: {
        const double rho = std::sqrt(u * u + v * v);
        const double x = M_PI * std::max(m_AntennaWidth, m_AntennaHeight) / lambda * rho;
        if (std::fabs(x) < 1.0e-12) { return 1.0; }
        const double approxJ1 = std::sin(x) / (x * x) - std::cos(x) / x;
        return std::fabs(2.0 * approxJ1 / x);
    }

    case SelectedPattern::CosineSquaredPedestal: {
        const double pedestal1 = m_Factor1;
        const double pedestal2 = m_Factor2;
        double caz = std::cos(0.5 * dAzRad);
        double cel = std::cos(0.5 * dElRad);
        if (caz < 0.0) { caz = 0.0; }
        if (cel < 0.0) { cel = 0.0; }
        const double waz = pedestal1 + pedestal2 * caz * caz;
        const double wel = pedestal1 + pedestal2 * cel * cel;
        return base * std::max(0.0, waz) * std::max(0.0, wel);
    }

    case SelectedPattern::Taylor: {
        const double sidelobeLinear = std::pow(10.0, m_Sidelobe_Levels / 20.0);
        const double order = std::max(1, m_nBar);
        const double rho = std::sqrt(u * u + v * v);
        double w = sidelobeLinear + (1.0 - sidelobeLinear) * std::pow(std::max(0.0, 1.0 - rho), order);
        if (w < 0.0) { w = 0.0; }
        return base * w;
    }

    default:
        return base;
    }
}
