#include "RADAR_Antenna_Rx_Block.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <complex>
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

bool RADAR_Antenna_Rx_Block::parseArrayString(const std::string &arrayStr, std::vector<double> &outArray)
{
        outArray.clear();

    std::string str = arrayStr;
    // 去除首尾空格
    size_t start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return false;
    size_t end = str.find_last_not_of(" \t\n\r");
    str = str.substr(start, end - start + 1);

    // 检查是否是数组格式
    if (str.empty() || str.front() != '[' || str.back() != ']') {
        return false;
    }

    // 去除外层括号
    std::string content = str.substr(1, str.length() - 2);

    // 去除首尾空格
    start = content.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) {
        // 空数组
        return true;
    }
    end = content.find_last_not_of(" \t\n\r");
    content = content.substr(start, end - start + 1);

    // 按逗号分割
    std::stringstream ss(content);
    std::string item;

    while (std::getline(ss, item, ',')) {
        // 去除空格
        start = item.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) continue;
        end = item.find_last_not_of(" \t\n\r");
        item = item.substr(start, end - start + 1);

        if (!item.empty()) {
            try {
                int value = std::stoi(item);
                outArray.push_back(value);
            } catch (const std::exception& e) {
                LOG_ERROR("Failed to parse array element: ", item, " - ", e.what());
                return false;
            }
        }
    }

    return true;
}

RADAR_Antenna_Rx_Block::RADAR_Antenna_Rx_Block(const std::string& name)
    : Block(name)
{
}

void RADAR_Antenna_Rx_Block::SetDefaultParamters()
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
    m_NumberOfRasterBars = 0;
    m_RasterBarWidth = 5.0;
    m_TargetAzimuthAngle = nullptr;
    m_TargetAzimuthAngle_Size = 0;
    m_TargetElevationAngle = nullptr;
    m_TargetElevationAngle_Size = 0;
    m_BeamAzimuthAngle = 0.0;
    m_BeamElevationAngle = 0.0;
}

void RADAR_Antenna_Rx_Block::SetParameters()
{
    m_AntennaPatternArray = primdata.data();
    m_AntennaPatternArray_Size =  static_cast<double>(primdata.size());
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
    m_ant->NumberOfRasterBars = m_NumberOfRasterBars;
    m_ant->RasterBarWidth = m_RasterBarWidth;
    m_ant->TargetAzimuthAngle = m_TargetAzimuthAngle;
    m_ant->TargetAzimuthAngle_Size = m_TargetAzimuthAngle_Size;
    m_ant->TargetElevationAngle = m_TargetElevationAngle;
    m_ant->TargetElevationAngle_Size = m_TargetElevationAngle_Size;
    m_ant->BeamAzimuthAngle = m_BeamAzimuthAngle;
    m_ant->BeamElevationAngle = m_BeamElevationAngle;
}

bool RADAR_Antenna_Rx_Block::Setup()
{
    Block::Setup();

    m_inputBuffer.clear();
    while (!m_outputQueue.empty()) m_outputQueue.pop();

    SetParameters();
    return true;
}

bool RADAR_Antenna_Rx_Block::Run()
{
    if (IsVariableStepMode()) {
        return TimeDrivenRun();
    }
    return DataStreamRun();
}

bool RADAR_Antenna_Rx_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_ant = std::make_unique<RADAR_Antenna_Rx>();
    SetDefaultParamters();

    try { m_RadarWorkMode = ConvertStringToRadarWorkMode(getParameter("RadarWorkMode").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'RadarWorkMode', using default value."); }
    try { m_Pattern = ConvertStringToPattern(getParameter("Pattern").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Pattern', using default value."); }
    try { m_Factor1 = std::stod(getParameter("Factor1").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Factor1', using default value."); }
    try { m_Factor2 = std::stod(getParameter("Factor2").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Factor2', using default value."); }
    try {
        std::string PrimString = getParameter("AntennaPatternArray").Value;
        parseArrayString(PrimString, primdata);
    } catch(...) { LOG_WARN("Failed to parse parameter 'AntennaPatternArray', using default value."); }
    try { m_Sidelobe_Levels = std::stod(getParameter("Sidelobe_Levels").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Sidelobe_Levels', using default value."); }
    try { m_nBar = std::stoi(getParameter("nBar").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'nBar', using default value."); }
    try { m_AntennaHeight = std::stod(getParameter("AntennaHeight").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'AntennaHeight', using default value."); }
    try { m_AntennaWidth = std::stod(getParameter("AntennaWidth").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'AntennaWidth', using default value."); }
    try { m_AntennaScanPattern = ConvertStringToAntennaScanPattern(getParameter("AntennaScanPattern").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'AntennaScanPattern', using default value."); }
    try { m_ScanRate = std::stod(getParameter("ScanRate").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'ScanRate', using default value."); }
    try { m_ElevationAngle = std::stod(getParameter("ElevationAngle").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'ElevationAngle', using default value."); }
    try { m_SectorScanStartAngle = std::stod(getParameter("SectorScanStartAngle").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'SectorScanStartAngle', using default value."); }
    try { m_SectorScanEndAngle = std::stod(getParameter("SectorScanEndAngle").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'SectorScanEndAngle', using default value."); }
    try { m_FlybackTime = std::stod(getParameter("FlybackTime").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'FlybackTime', using default value."); }
    try { m_NumberOfRasterBars = std::stoi(getParameter("NumberOfRasterBars").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'NumberOfRasterBars', using default value."); }
    try { m_RasterBarWidth = std::stod(getParameter("RasterBarWidth").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'RasterBarWidth', using default value."); }
    try { m_BeamAzimuthAngle = std::stod(getParameter("BeamAzimuthAngle").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'BeamAzimuthAngle', using default value."); }
    try { m_BeamElevationAngle = std::stod(getParameter("BeamElevationAngle").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'BeamElevationAngle', using default value."); }

    SetParameters();

    AddInputPort("TargetAzimuth",   m_ant->TargetAzimuth,   1, Block::DataType::DOUBLE_BUS);
    AddInputPort("TargetElevation", m_ant->TargetElevation, 1, Block::DataType::DOUBLE_BUS);
    AddInputPort("BeamAzimuth",     m_ant->BeamAzimuth,     1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddInputPort("BeamElevation",   m_ant->BeamElevation,   1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddInputPort("input",           m_ant->input,           1, Block::DataType::ENVELOPE_BUS);
    AddOutputPort("output",         m_ant->output,          1, Block::DataType::ENVELOPE_SIGNAL);

    return true;
}

bool RADAR_Antenna_Rx_Block::DataStreamRun()
{
     auto inputData = ReadInputData<EnvelopeSignal>(GetInputPortName(4));
     if (inputData.empty()) { return false; }

     const int nTarget = static_cast<int>(inputData.size());

     double fcHz = 0.0;
     {
         auto* reader = GetInputPort(GetInputPortName(4));
         if (reader && reader->hasCharacterizationFrequency()) {
             fcHz = reader->getCharacterizationFrequency();
         }
     }

     auto azData  = ReadInputData<double>(GetInputPortName(0));
     auto elData  = ReadInputData<double>(GetInputPortName(1));
     auto bAzData = ReadInputData<double>(GetInputPortName(2));
     auto bElData = ReadInputData<double>(GetInputPortName(3));

     bool hasTargetAzPort = !azData.empty();
     bool hasTargetElPort = !elData.empty();
     bool hasBeamAzPort   = !bAzData.empty();
     bool hasBeamElPort   = !bElData.empty();

     double beamAzRad = 0.0, beamElRad = 0.0;

     if (hasBeamAzPort) {
         beamAzRad = bAzData[0];
     }
     if (hasBeamElPort) {
         beamElRad = bElData[0];
     }
     if (!hasBeamAzPort || !hasBeamElPort) {
         double timeNow = 0.0;
         double azTmp = 0.0, elTmp = 0.0;
         getBeamAngle(timeNow, azTmp, elTmp);
         if (!hasBeamAzPort) { beamAzRad = azTmp; }
         if (!hasBeamElPort) { beamElRad = elTmp; }
     }

     const double apertureGain = calcApertureGainLinear(fcHz);
     std::complex<double> y(0.0, 0.0);

     for (int ch = 0; ch < nTarget; ++ch) {
         double tAz = hasTargetAzPort
             ? (ch < static_cast<int>(azData.size()) ? azData[ch] : 0.0)
             : deg2rad(getArrayValue(m_TargetAzimuthAngle, m_TargetAzimuthAngle_Size, ch, 0.0));
         double tEl = hasTargetElPort
             ? (ch < static_cast<int>(elData.size()) ? elData[ch] : 0.0)
             : deg2rad(getArrayValue(m_TargetElevationAngle, m_TargetElevationAngle_Size, ch, 0.0));

         const double sep = angularSeparation(tAz, tEl, beamAzRad, beamElRad);
         if (sep > 0.5 * M_PI) { continue; }

         const double patternGainDb = calcPatternGainDb(tAz, tEl, beamAzRad, beamElRad, fcHz);
         const double patternGain = std::pow(10.0, patternGainDb / 20.0);

         std::complex<double> xin(0.0, 0.0);
         if (ch < nTarget) { xin = inputData[ch].complex(); }

         y += xin * apertureGain * patternGain;

     }
     WriteOutputData(GetOutputPortName(0), std::vector<EnvelopeSignal>{EnvelopeSignal(y)});
     return true;
}

// ============================================================================
// TimeDrivenRun — 变步长模式：输入存 buffer，处理后入队，队列非空则输出
// ============================================================================

bool RADAR_Antenna_Rx_Block::TimeDrivenRun()
{
    // ① 累积输入
    {
        auto inputData = ReadInputData<EnvelopeSignal>(GetInputPortName(4));
        if (inputData.empty()) {
            // 无新数据，仅尝试出队输出
            goto output_label;
        }

        double fcHz = 0.0;
        {
            auto* reader = GetInputPort(GetInputPortName(4));
            if (reader && reader->hasCharacterizationFrequency()) {
                fcHz = reader->getCharacterizationFrequency();
            }
        }

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
        in.inputSignals.assign(inputData.begin(), inputData.end());
        in.fcHz = fcHz;
        m_inputBuffer.push_back(in);
    }

    // ② 处理所有累积输入 → 入队
    if (!m_inputBuffer.empty()) {
        InputSnapshot in = m_inputBuffer.front();
        m_inputBuffer.erase(m_inputBuffer.begin());

        const int nTarget = static_cast<int>(in.inputSignals.size());

        bool hasTargetAzPort = !in.targetAz.empty();
        bool hasTargetElPort = !in.targetEl.empty();
        bool hasBeamAzPort   = in.hasBeamAzPort;
        bool hasBeamElPort   = in.hasBeamElPort;

        double beamAzRad = 0.0, beamElRad = 0.0;
        if (hasBeamAzPort) { beamAzRad = in.beamAz; }
        if (hasBeamElPort) { beamElRad = in.beamEl; }

        if (!hasBeamAzPort || !hasBeamElPort) {
            double timeNow = 0.0;
            double azTmp = 0.0, elTmp = 0.0;
            getBeamAngle(timeNow, azTmp, elTmp);
            if (!hasBeamAzPort) { beamAzRad = azTmp; }
            if (!hasBeamElPort) { beamElRad = elTmp; }
        }

        const double apertureGain = calcApertureGainLinear(in.fcHz);
        std::complex<double> y(0.0, 0.0);

        for (int ch = 0; ch < nTarget; ++ch) {
            double tAz = hasTargetAzPort
                ? (ch < static_cast<int>(in.targetAz.size()) ? in.targetAz[ch] : 0.0)
                : deg2rad(getArrayValue(m_TargetAzimuthAngle, m_TargetAzimuthAngle_Size, ch, 0.0));
            double tEl = hasTargetElPort
                ? (ch < static_cast<int>(in.targetEl.size()) ? in.targetEl[ch] : 0.0)
                : deg2rad(getArrayValue(m_TargetElevationAngle, m_TargetElevationAngle_Size, ch, 0.0));

            const double sep = angularSeparation(tAz, tEl, beamAzRad, beamElRad);
            if (sep > 0.5 * M_PI) { continue; }

            const double patternGainDb = calcPatternGainDb(tAz, tEl, beamAzRad, beamElRad, in.fcHz);
            const double patternGain = std::pow(10.0, patternGainDb / 20.0);

            std::complex<double> xin(0.0, 0.0);
            if (ch < nTarget) { xin = in.inputSignals[ch].complex(); }

            y += xin * apertureGain * patternGain;
        }

        OutputFrame outFrame;
        outFrame.out = EnvelopeSignal(y);
        m_outputQueue.push(outFrame);
    }

    // ③ 出队写入：outputQueue 不为空就输出一次
output_label:
    if (!m_outputQueue.empty()) {
        OutputFrame outFrame = m_outputQueue.front();
        m_outputQueue.pop();
        m_inputBuffer.clear();
        WriteOutputData(GetOutputPortName(0), std::vector<EnvelopeSignal>{outFrame.out});
    }

    return true;
}


RADAR_Antenna_Rx_Block::SelectedRadarWorkMode
RADAR_Antenna_Rx_Block::ConvertStringToRadarWorkMode(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "tracking" || lower == "0") return SelectedRadarWorkMode::Tracking;
    if (lower == "search" || lower == "1")   return SelectedRadarWorkMode::Search;
    return SelectedRadarWorkMode::Tracking;
}

RADAR_Antenna_Rx_Block::SelectedPattern
RADAR_Antenna_Rx_Block::ConvertStringToPattern(const std::string& value)
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

RADAR_Antenna_Rx_Block::SelectedAntennaScanPattern
RADAR_Antenna_Rx_Block::ConvertStringToAntennaScanPattern(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "circular" || lower == "0")                 return SelectedAntennaScanPattern::CircularScan;
    if (lower == "bidirectional sector" || lower == "1")     return SelectedAntennaScanPattern::BidirectionalSector;
    if (lower == "unidirectional sector" || lower == "2")    return SelectedAntennaScanPattern::UnidirectionalSector;
    if (lower == "bidirectional raster" || lower == "3")     return SelectedAntennaScanPattern::BidirectionalRaster;
    if (lower == "unidirectional raster" || lower == "4")    return SelectedAntennaScanPattern::UnidirectionalRaster;
    return SelectedAntennaScanPattern::CircularScan;
}

double RADAR_Antenna_Rx_Block::deg2rad(double x) { return x * M_PI / 180.0; }
double RADAR_Antenna_Rx_Block::rad2deg(double x) { return x * 180.0 / M_PI; }

double RADAR_Antenna_Rx_Block::wrapToPi(double x)
{
    while (x > M_PI)       { x -= 2.0 * M_PI; }
    while (x < -M_PI)      { x += 2.0 * M_PI; }
    return x;
}

double RADAR_Antenna_Rx_Block::wrapTo360(double x)
{
    double y = std::fmod(x, 360.0);
    if (y < 0.0) { y += 360.0; }
    return y;
}

double RADAR_Antenna_Rx_Block::clampValue(double v, double lo, double hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

double RADAR_Antenna_Rx_Block::sinc(double x)
{
    if (std::fabs(x) < 1.0e-12) return 1.0;
    return std::sin(x) / x;
}

double RADAR_Antenna_Rx_Block::besselI0(double x)
{
    double sum = 1.0, term = 1.0;
    const double xx = 0.25 * x * x;
    for (int k = 1; k < 40; ++k) {
        term *= xx / static_cast<double>(k * k);
        sum += term;
        if (std::fabs(term) < 1.0e-14 * std::fabs(sum)) break;
    }
    return sum;
}

double RADAR_Antenna_Rx_Block::getArrayValue(const double* data, int size, int index, double defaultValue) const
{
    if (data == nullptr || size <= 0) return defaultValue;
    if (index < 0)  return data[0];
    if (index < size) return data[index];
    return data[size - 1];
}

void RADAR_Antenna_Rx_Block::getBeamAngle(double timeNow, double& beamAzRad, double& beamElRad)
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
            beamElRad = deg2rad(elDeg);
        } else if (m_AntennaScanPattern == SelectedAntennaScanPattern::UnidirectionalRaster) {
            double azDeg = 0.0, elDeg = 0.0;
            getRasterScanAngle(timeNow, false, azDeg, elDeg);
            beamAzRad = deg2rad(azDeg);
            beamElRad = deg2rad(elDeg);
        }
    } else {
        beamAzRad = deg2rad(m_BeamAzimuthAngle);
    }

    if (m_RadarWorkMode == SelectedRadarWorkMode::Search) {
        if (m_AntennaScanPattern == SelectedAntennaScanPattern::BidirectionalRaster ||
            m_AntennaScanPattern == SelectedAntennaScanPattern::UnidirectionalRaster) {
            double azDeg = 0.0, elDeg = 0.0;
            getRasterScanAngle(timeNow,
                m_AntennaScanPattern == SelectedAntennaScanPattern::BidirectionalRaster,
                azDeg, elDeg);
            beamElRad = deg2rad(elDeg);
        } else {
            beamElRad = deg2rad(m_ElevationAngle);
        }
    } else {
        beamElRad = deg2rad(m_BeamElevationAngle);
    }
}

double RADAR_Antenna_Rx_Block::getCircularScanAzimuth(double timeNow) const
{
    const double rateDegPerSec = m_ScanRate * 6.0;
    return wrapTo360(rateDegPerSec * timeNow);
}

double RADAR_Antenna_Rx_Block::getSectorScanAzimuth(double timeNow, bool bidirectional) const
{
    const double startDeg = m_SectorScanStartAngle;
    const double endDeg   = m_SectorScanEndAngle;
    const double span = std::fabs(endDeg - startDeg);
    const double dir  = (endDeg >= startDeg) ? 1.0 : -1.0;
    const double rate = std::fabs(m_ScanRate * 6.0);

    if (span <= 1.0e-12 || rate <= 1.0e-12) return startDeg;

    const double oneWayTime = span / rate;

    if (bidirectional) {
        const double period = 2.0 * oneWayTime;
        double t = std::fmod(timeNow, period);
        if (t < 0.0) t += period;
        if (t <= oneWayTime) return startDeg + dir * rate * t;
        return endDeg - dir * rate * (t - oneWayTime);
    } else {
        const double fb = (m_FlybackTime > 0.0) ? m_FlybackTime : 0.0;
        const double period = oneWayTime + fb;
        double t = std::fmod(timeNow, period);
        if (t < 0.0) t += period;
        if (t <= oneWayTime) return startDeg + dir * rate * t;
        return startDeg;
    }
}

void RADAR_Antenna_Rx_Block::getRasterScanAngle(double timeNow, bool bidirectional,
                                                  double& azDeg, double& elDeg) const
{
    const int bars = (m_NumberOfRasterBars > 0) ? m_NumberOfRasterBars : 1;
    const double startDeg = m_SectorScanStartAngle;
    const double endDeg   = m_SectorScanEndAngle;
    const double span = std::fabs(endDeg - startDeg);
    const double dir  = (endDeg >= startDeg) ? 1.0 : -1.0;
    const double rate = std::fabs(m_ScanRate * 6.0);

    if (span <= 1.0e-12 || rate <= 1.0e-12) {
        azDeg = startDeg;
        elDeg = m_ElevationAngle;
        return;
    }

    const double oneBarTime = span / rate;

    if (bidirectional) {
        const double totalTime = oneBarTime * static_cast<double>(bars);
        double t = std::fmod(timeNow, totalTime);
        if (t < 0.0) t += totalTime;
        int bar = static_cast<int>(std::floor(t / oneBarTime));
        if (bar < 0) bar = 0;
        if (bar >= bars) bar = bars - 1;
        const double localT = t - static_cast<double>(bar) * oneBarTime;
        const bool reverse = (bar % 2) != 0;
        azDeg = reverse ? (endDeg - dir * rate * localT) : (startDeg + dir * rate * localT);
        elDeg = m_ElevationAngle + static_cast<double>(bar) * m_RasterBarWidth;
    } else {
        const double fb = (m_FlybackTime > 0.0) ? m_FlybackTime : 0.0;
        const double oneCycle = oneBarTime + fb;
        const double totalTime = oneCycle * static_cast<double>(bars);
        double t = std::fmod(timeNow, totalTime);
        if (t < 0.0) t += totalTime;
        int bar = static_cast<int>(std::floor(t / oneCycle));
        if (bar < 0) bar = 0;
        if (bar >= bars) bar = bars - 1;
        const double localT = t - static_cast<double>(bar) * oneCycle;
        azDeg = (localT <= oneBarTime) ? (startDeg + dir * rate * localT) : startDeg;
        elDeg = m_ElevationAngle + static_cast<double>(bar) * m_RasterBarWidth;
    }
}

double RADAR_Antenna_Rx_Block::angularSeparation(double az1, double el1,
                                                   double az2, double el2) const
{
    const double c1 = std::cos(el1), c2 = std::cos(el2);
    const double x1 = c1 * std::cos(az1);
    const double y1 = c1 * std::sin(az1);
    const double z1 = std::sin(el1);
    const double x2 = c2 * std::cos(az2);
    const double y2 = c2 * std::sin(az2);
    const double z2 = std::sin(el2);
    const double dot = clampValue(x1 * x2 + y1 * y2 + z1 * z2, -1.0, 1.0);
    return std::acos(dot);
}

double RADAR_Antenna_Rx_Block::calcPatternGainDb(double targetAzRad, double targetElRad,
                                                   double beamAzRad, double beamElRad,
                                                   double fcHz) const
{
    const double dAz = wrapToPi(targetAzRad - beamAzRad);
    const double dEl = targetElRad - beamElRad;
    if (m_Pattern == SelectedPattern::UserDefinedPattern) {
        return calcUserPatternGainDb(dAz, dEl);
    }
    return calcAnalyticPatternGainDb(dAz, dEl, fcHz);
}

double RADAR_Antenna_Rx_Block::calcUserPatternGainDb(double dAzRad, double dElRad) const
{
    if (m_AntennaPatternArray == nullptr || m_AntennaPatternArray_Size <= 0) {
        return 0.0;
    }
    const double azDeg = wrapTo360(rad2deg(dAzRad) + 180.0);
    const double elDeg = clampValue(rad2deg(dElRad) + 90.0, 0.0, 179.999999);
    const int col = static_cast<int>(std::floor(azDeg));
    const int row = static_cast<int>(std::floor(elDeg));
    const int idx = row * 360 + col;
    if (idx >= 0 && idx < m_AntennaPatternArray_Size) {
        return m_AntennaPatternArray[idx];
    }
    const int safeIdx = static_cast<int>(clampValue(static_cast<double>(idx), 0.0,
                                                     static_cast<double>(m_AntennaPatternArray_Size - 1)));
    return m_AntennaPatternArray[safeIdx];
}

double RADAR_Antenna_Rx_Block::calcAnalyticPatternGainDb(double dAzRad, double dElRad,
                                                          double fcHz) const
{
    const double c0 = 3.0e8;
    double lambda = 1.0;
    if (fcHz > 1.0) { lambda = c0 / fcHz; }
    const double width  = (m_AntennaWidth > 1.0e-12)  ? m_AntennaWidth  : 1.0e-12;
    const double height = (m_AntennaHeight > 1.0e-12) ? m_AntennaHeight : 1.0e-12;
    const double ux = M_PI * width  / lambda * std::sin(dAzRad);
    const double uy = M_PI * height / lambda * std::sin(dElRad);
    double amp = std::fabs(sinc(ux) * sinc(uy));
    const double azNorm = clampValue(std::fabs(dAzRad) / (0.5 * M_PI), 0.0, 1.0);
    const double elNorm = clampValue(std::fabs(dElRad) / (0.5 * M_PI), 0.0, 1.0);
    const double uNorm  = clampValue(std::sqrt(azNorm * azNorm + elNorm * elNorm), 0.0, 1.0);
    amp *= calcDistributionWeight(uNorm);
    if (amp < 1.0e-300) { amp = 1.0e-300; }
    return 20.0 * std::log10(amp);
}

double RADAR_Antenna_Rx_Block::calcDistributionWeight(double uNorm) const
{
    const double u = clampValue(uNorm, 0.0, 1.0);
    switch (m_Pattern) {
    case SelectedPattern::UserDefinedPattern:
    case SelectedPattern::Uniform:
        return 1.0;
    case SelectedPattern::Cosine: {
        const double n = (m_Factor1 > 0.0) ? m_Factor1 : 1.0;
        const double w = std::cos(0.5 * M_PI * u);
        return std::pow(clampValue(w, 0.0, 1.0), n);
    }
    case SelectedPattern::Parabolic: {
        const double n = (m_Factor1 > 0.0) ? m_Factor1 : 1.0;
        const double w = 1.0 - u * u;
        return std::pow(clampValue(w, 0.0, 1.0), n);
    }
    case SelectedPattern::Triangle: {
        const double n = (m_Factor1 > 0.0) ? m_Factor1 : 1.0;
        const double w = 1.0 - u;
        return std::pow(clampValue(w, 0.0, 1.0), n);
    }
    case SelectedPattern::Circular: {
        const double n = (m_Factor1 > 0.0) ? m_Factor1 : 1.0;
        const double w = std::sqrt(clampValue(1.0 - u * u, 0.0, 1.0));
        return std::pow(clampValue(w, 0.0, 1.0), n);
    }
    case SelectedPattern::CosineSquaredPedestal: {
        double pedestal = clampValue(m_Factor1, 0.0, 1.0);
        const double n = (m_Factor2 > 0.0) ? m_Factor2 : 1.0;
        const double c = std::cos(0.5 * M_PI * u);
        const double taper = std::pow(clampValue(c * c, 0.0, 1.0), n);
        return pedestal + (1.0 - pedestal) * taper;
    }
    case SelectedPattern::Taylor: {
        const double atten = std::fabs(m_Sidelobe_Levels);
        const double beta = (atten > 0.0) ? std::sqrt(atten) : 0.0;
        const double arg = beta * std::sqrt(clampValue(1.0 - u * u, 0.0, 1.0));
        double denom = besselI0(beta);
        if (std::fabs(denom) < 1.0e-300) { denom = 1.0; }
        double w = besselI0(arg) / denom;
        if (m_nBar > 1) {
            w = std::pow(clampValue(w, 0.0, 1.0), 1.0 / static_cast<double>(m_nBar));
        }
        return clampValue(w, 0.0, 1.0);
    }
    default:
        return 1.0;
    }
}

double RADAR_Antenna_Rx_Block::calcApertureGainLinear(double fcHz) const
{
    if (m_Pattern == SelectedPattern::UserDefinedPattern) {
        return 1.0;
    }
    if (fcHz <= 1.0) {
        return 1.0;
    }
    const double c0 = 3.0e8;
    const double height = (m_AntennaHeight > 1.0e-12) ? m_AntennaHeight : 1.0e-12;
    const double width  = (m_AntennaWidth  > 1.0e-12) ? m_AntennaWidth  : 1.0e-12;
    const double lambda = c0 / fcHz;
    const double area = height * width;
    double gain = std::sqrt(4.0 * M_PI) * area / (lambda * lambda);
    if (gain < 0.0 || gain != gain) { gain = 1.0; }
    return gain;
}
