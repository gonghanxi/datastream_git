#include "COMM_AntennaPolarizationTx_Block.h"
#include <cmath>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>

#ifndef M_PI
#define M_PI 3.14159265358979323846
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

std::vector<double> ParseDoubleArray(const std::string& value)
{
    std::string s = TrimCopy(value);
    if(!s.empty() && s.front() == '[') s = s.substr(1);
    if(!s.empty() && s.back() == ']') s.pop_back();

    std::vector<double> result;
    std::string token;
    for(char c : s) {
        if(c == ',') {
            std::string t = TrimCopy(token);
            if(!t.empty()) result.push_back(std::stod(t));
            token.clear();
        } else {
            token += c;
        }
    }
    std::string t = TrimCopy(token);
    if(!t.empty()) result.push_back(std::stod(t));
    return result;
}
}

// ============================================================================
// 构造函数 / 析构函数
// ============================================================================

COMM_AntennaPolarizationTx_Block::COMM_AntennaPolarizationTx_Block(const std::string& name)
    : Block(name)
{
}

COMM_AntennaPolarizationTx_Block::~COMM_AntennaPolarizationTx_Block()
{
    m_algo.reset();
}

// ============================================================================
// Setup
// ============================================================================

bool COMM_AntennaPolarizationTx_Block::Setup()
{
    Block::Setup();
    while(!m_outputQueue.empty()) m_outputQueue.pop();
    return true;
}

// ============================================================================
// Run — 运行模式分发
// ============================================================================

bool COMM_AntennaPolarizationTx_Block::Run()
{
    if(!CanProcess()) {
        return false;
    }

    if(IsVariableStepMode()) {
        return TimeDrivenRun();
    }
    return DataStreamRun();
}

// ============================================================================
// Initialize — 模块初始化
// ============================================================================

bool COMM_AntennaPolarizationTx_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_algo = std::make_unique<COMM_AntennaPolarizationTx>();

    SetDefaultParameters();

    // ========== 解析参数 ==========
    try { m_PatternDataMode = ConvertStringToEnum(getParameter("PatternDataMode").Value, 0); } catch (...) { LOG_WARN("Failed to parse 'PatternDataMode'."); }
    try { m_ParametricPatternType = ConvertStringToEnum(getParameter("ParametricPatternType").Value, 2); } catch (...) { LOG_WARN("Failed to parse 'ParametricPatternType'."); }
    try { m_PeakGain_dBi = std::stod(getParameter("PeakGain_dBi").Value); } catch (...) { LOG_WARN("Failed to parse 'PeakGain_dBi'."); }
    try { m_AzimuthHPBW = std::stod(getParameter("AzimuthHPBW").Value); } catch (...) { LOG_WARN("Failed to parse 'AzimuthHPBW'."); }
    try { m_ElevationHPBW = std::stod(getParameter("ElevationHPBW").Value); } catch (...) { LOG_WARN("Failed to parse 'ElevationHPBW'."); }
    try { m_MaxAttenuation_dB = std::stod(getParameter("MaxAttenuation_dB").Value); } catch (...) { LOG_WARN("Failed to parse 'MaxAttenuation_dB'."); }
    try { m_VerticalSidelobeAttenuation_dB = std::stod(getParameter("VerticalSidelobeAttenuation_dB").Value); } catch (...) { LOG_WARN("Failed to parse 'VerticalSidelobeAttenuation_dB'."); }

    try { m_PolarizationType = ConvertStringToEnum(getParameter("PolarizationType").Value, 2); } catch (...) { LOG_WARN("Failed to parse 'PolarizationType'."); }
    try { m_PolarizationTiltAngle = std::stod(getParameter("PolarizationTiltAngle").Value); } catch (...) { LOG_WARN("Failed to parse 'PolarizationTiltAngle'."); }
    try { m_XPD_dB = std::stod(getParameter("XPD_dB").Value); } catch (...) { LOG_WARN("Failed to parse 'XPD_dB'."); }
    try { m_CrossPolarPhaseAngle = std::stod(getParameter("CrossPolarPhaseAngle").Value); } catch (...) { LOG_WARN("Failed to parse 'CrossPolarPhaseAngle'."); }

    try { m_UserJonesHMagnitude = std::stod(getParameter("UserJonesHMagnitude").Value); } catch (...) { LOG_WARN("Failed to parse 'UserJonesHMagnitude'."); }
    try { m_UserJonesHPhase = std::stod(getParameter("UserJonesHPhase").Value); } catch (...) { LOG_WARN("Failed to parse 'UserJonesHPhase'."); }
    try { m_UserJonesVMagnitude = std::stod(getParameter("UserJonesVMagnitude").Value); } catch (...) { LOG_WARN("Failed to parse 'UserJonesVMagnitude'."); }
    try { m_UserJonesVPhase = std::stod(getParameter("UserJonesVPhase").Value); } catch (...) { LOG_WARN("Failed to parse 'UserJonesVPhase'."); }

    try { m_ElementPatternFileType = ConvertStringToEnum(getParameter("ElementPatternFileType").Value, 0); } catch (...) { LOG_WARN("Failed to parse 'ElementPatternFileType'."); }
    try { m_ElementPatternFileScaleFactor = ParseDoubleArray(getParameter("ElementPatternFileScaleFactor").Value); } catch (...) { LOG_WARN("Failed to parse 'ElementPatternFileScaleFactor'."); }
    try { m_ImportedPatternDimension = ConvertStringToEnum(getParameter("ImportedPatternDimension").Value, 1); } catch (...) { LOG_WARN("Failed to parse 'ImportedPatternDimension'."); }
    try { m_ImportedGainMode = ConvertStringToEnum(getParameter("ImportedGainMode").Value, 0); } catch (...) { LOG_WARN("Failed to parse 'ImportedGainMode'."); }
    try { m_TxAntennaPatternFileName = getParameter("TxAntennaPatternFileName1").Value; } catch (...) { LOG_WARN("Failed to parse 'TxAntennaPatternFileName1'."); }

    try { m_BeamControlMode = ConvertStringToEnum(getParameter("BeamControlMode").Value, 0); } catch (...) { LOG_WARN("Failed to parse 'BeamControlMode'."); }
    try { m_BeamScanPattern = ConvertStringToEnum(getParameter("BeamScanPattern").Value, 0); } catch (...) { LOG_WARN("Failed to parse 'BeamScanPattern'."); }
    try { m_ScanRate = std::stod(getParameter("ScanRate").Value); } catch (...) { LOG_WARN("Failed to parse 'ScanRate'."); }
    try { m_ElevationAngle = std::stod(getParameter("ElevationAngle").Value); } catch (...) { LOG_WARN("Failed to parse 'ElevationAngle'."); }
    try { m_SectorScanStartAngle = std::stod(getParameter("SectorScanStartAngle").Value); } catch (...) { LOG_WARN("Failed to parse 'SectorScanStartAngle'."); }
    try { m_SectorScanEndAngle = std::stod(getParameter("SectorScanEndAngle").Value); } catch (...) { LOG_WARN("Failed to parse 'SectorScanEndAngle'."); }
    try { m_FlybackTime = std::stod(getParameter("FlybackTime").Value); } catch (...) { LOG_WARN("Failed to parse 'FlybackTime'."); }
    try { m_NumberOfRasterBars = std::stoi(getParameter("NumberOfRasterBars").Value); } catch (...) { LOG_WARN("Failed to parse 'NumberOfRasterBars'."); }
    try { m_RasterBarWidth = std::stod(getParameter("RasterBarWidth").Value); } catch (...) { LOG_WARN("Failed to parse 'RasterBarWidth'."); }

    try { m_DirectionAzimuthAngle = ParseDoubleArray(getParameter("DirectionAzimuthAngle").Value); } catch (...) { LOG_WARN("Failed to parse 'DirectionAzimuthAngle'."); }
    try { m_DirectionElevationAngle = ParseDoubleArray(getParameter("DirectionElevationAngle").Value); } catch (...) { LOG_WARN("Failed to parse 'DirectionElevationAngle'."); }
    try { m_BeamAzimuthAngle = std::stod(getParameter("BeamAzimuthAngle").Value); } catch (...) { LOG_WARN("Failed to parse 'BeamAzimuthAngle'."); }
    try { m_BeamElevationAngle = std::stod(getParameter("BeamElevationAngle").Value); } catch (...) { LOG_WARN("Failed to parse 'BeamElevationAngle'."); }

    // 设置参数到算法实例（仅用于端口注册）
    SetParameters();

    // Block 自行初始化
    if(!ModelSetup()) return false;

    // ========== 注册端口 ==========
    // 输入端口
    AddInputPort("input", m_algo->input, 1, DataType::ENVELOPE_SIGNAL);
    AddInputPort("DirectionAzimuth", m_algo->DirectionAzimuth, 1, DataType::DOUBLE_BUS);
    AddInputPort("DirectionElevation", m_algo->DirectionElevation, 1, DataType::DOUBLE_BUS);
    AddInputPort("BeamAzimuth", m_algo->BeamAzimuth, 1, DataType::CIRCULAR_BUFFER_DOUBLE);
    AddInputPort("BeamElevation", m_algo->BeamElevation, 1, DataType::CIRCULAR_BUFFER_DOUBLE);

    // 输出端口（bus，writeSize=1 表示每个 bus 通道的速率）
    AddOutputPort("output_V", m_algo->output_V, 1, DataType::ENVELOPE_BUS);
    AddOutputPort("output_H", m_algo->output_H, 1, DataType::ENVELOPE_BUS);

    return true;
}

// ============================================================================
// SetDefaultParameters
// ============================================================================

void COMM_AntennaPolarizationTx_Block::SetDefaultParameters()
{
    m_PatternDataMode = 0; // ParametricPattern
    m_ParametricPatternType = 2; // GaussianPattern
    m_PeakGain_dBi = 0.0;
    m_AzimuthHPBW = 65.0;
    m_ElevationHPBW = 65.0;
    m_MaxAttenuation_dB = 30.0;
    m_VerticalSidelobeAttenuation_dB = 30.0;

    m_PolarizationType = 2; // VerticalPolarization
    m_PolarizationTiltAngle = 45.0;
    m_XPD_dB = 40.0;
    m_CrossPolarPhaseAngle = 0.0;
    m_UserJonesHMagnitude = 1.0;
    m_UserJonesHPhase = 0.0;
    m_UserJonesVMagnitude = 0.0;
    m_UserJonesVPhase = 0.0;

    m_ElementPatternFileType = 0; // EMPro
    m_ElementPatternFileScaleFactor = {1.0};
    m_ImportedPatternDimension = 1; // UserDefine3D
    m_ImportedGainMode = 0; // UseFileGain
    m_TxAntennaPatternFileName = "";

    m_BeamControlMode = 0; // FixedBeam
    m_BeamScanPattern = 0; // CircularScan
    m_ScanRate = 15.0;
    m_ElevationAngle = 0.0;
    m_SectorScanStartAngle = -60.0;
    m_SectorScanEndAngle = 60.0;
    m_FlybackTime = 0.0;
    m_NumberOfRasterBars = 0;
    m_RasterBarWidth = 5.0;

    m_DirectionAzimuthAngle = {0.0};
    m_DirectionElevationAngle = {0.0};
    m_BeamAzimuthAngle = 0.0;
    m_BeamElevationAngle = 0.0;

    m_directionCount = 1;
    m_inputFc = 0.0;
}

// ============================================================================
// SetParameters — 同步参数到算法实例（仅用于端口注册）
// ============================================================================

void COMM_AntennaPolarizationTx_Block::SetParameters()
{
    if(!m_algo) return;

    m_algo->PatternDataMode = static_cast<COMM_AntennaPolarizationTx::SelectedPatternDataMode>(m_PatternDataMode);
    m_algo->ParametricPatternType = static_cast<COMM_AntennaPolarizationTx::SelectedParametricPatternType>(m_ParametricPatternType);
    m_algo->PeakGain_dBi = m_PeakGain_dBi;
    m_algo->AzimuthHPBW = m_AzimuthHPBW;
    m_algo->ElevationHPBW = m_ElevationHPBW;
    m_algo->MaxAttenuation_dB = m_MaxAttenuation_dB;
    m_algo->VerticalSidelobeAttenuation_dB = m_VerticalSidelobeAttenuation_dB;

    m_algo->PolarizationType = static_cast<COMM_AntennaPolarizationTx::SelectedPolarizationType>(m_PolarizationType);
    m_algo->PolarizationTiltAngle = m_PolarizationTiltAngle;
    m_algo->XPD_dB = m_XPD_dB;
    m_algo->CrossPolarPhaseAngle = m_CrossPolarPhaseAngle;
    m_algo->UserJonesHMagnitude = m_UserJonesHMagnitude;
    m_algo->UserJonesHPhase = m_UserJonesHPhase;
    m_algo->UserJonesVMagnitude = m_UserJonesVMagnitude;
    m_algo->UserJonesVPhase = m_UserJonesVPhase;

    m_algo->ElementPatternFileType = static_cast<COMM_AntennaPolarizationTx::SelectedElementPatternFileType>(m_ElementPatternFileType);
    m_algo->ImportedPatternDimension = static_cast<COMM_AntennaPolarizationTx::SelectedImportedPatternDimension>(m_ImportedPatternDimension);
    m_algo->ImportedGainMode = static_cast<COMM_AntennaPolarizationTx::SelectedImportedGainMode>(m_ImportedGainMode);

    if(!m_TxAntennaPatternFileName.empty()) {
        // 需要保持 C 字符串生命周期
        static std::string s_fileName;
        s_fileName = m_TxAntennaPatternFileName;
        m_algo->TxAntennaPatternFileName1 = const_cast<char*>(s_fileName.c_str());
    }

    m_algo->BeamControlMode = static_cast<COMM_AntennaPolarizationTx::SelectedBeamControlMode>(m_BeamControlMode);
    m_algo->BeamScanPattern = static_cast<COMM_AntennaPolarizationTx::SelectedBeamScanPattern>(m_BeamScanPattern);
    m_algo->ScanRate = m_ScanRate;
    m_algo->ElevationAngle = m_ElevationAngle;
    m_algo->SectorScanStartAngle = m_SectorScanStartAngle;
    m_algo->SectorScanEndAngle = m_SectorScanEndAngle;
    m_algo->FlybackTime = m_FlybackTime;
    m_algo->NumberOfRasterBars = m_NumberOfRasterBars;
    m_algo->RasterBarWidth = m_RasterBarWidth;

    // 数组参数
    if(!m_DirectionAzimuthAngle.empty()) {
        static std::vector<double> s_azAngles;
        s_azAngles = m_DirectionAzimuthAngle;
        m_algo->DirectionAzimuthAngle = s_azAngles.data();
        m_algo->DirectionAzimuthAngle_Size = static_cast<int>(s_azAngles.size());
    }
    if(!m_DirectionElevationAngle.empty()) {
        static std::vector<double> s_elAngles;
        s_elAngles = m_DirectionElevationAngle;
        m_algo->DirectionElevationAngle = s_elAngles.data();
        m_algo->DirectionElevationAngle_Size = static_cast<int>(s_elAngles.size());
    }
    m_algo->BeamAzimuthAngle = m_BeamAzimuthAngle;
    m_algo->BeamElevationAngle = m_BeamElevationAngle;

    if(!m_ElementPatternFileScaleFactor.empty()) {
        static std::vector<double> s_scaleFactors;
        s_scaleFactors = m_ElementPatternFileScaleFactor;
        m_algo->ElementPatternFileScaleFactor = s_scaleFactors.data();
        m_algo->ElementPatternFileScaleFactor_Size = static_cast<int>(s_scaleFactors.size());
    }
}

// ============================================================================
// ModelSetup — Block 自行初始化，不调用 m_algo->Setup()
// ============================================================================

bool COMM_AntennaPolarizationTx_Block::ModelSetup()
{
    // 配置校验
    if(!validateConfiguration()) {
        LOG_ERROR("COMM_AntennaPolarizationTx_Block: configuration validation failed.");
        return false;
    }

    // 加载方向图文件
    if(m_PatternDataMode == 1) { // ImportActualPattern
        if(!loadPatternFile()) {
            LOG_ERROR("COMM_AntennaPolarizationTx_Block: failed to load the selected antenna pattern file.");
            return false;
        }
    } else {
        clearPattern();
    }

    // 确定方向数（bus连接数在运行时才确定，这里先用参数数组大小估算）
    int n = 0;
    n = std::max(n, static_cast<int>(m_DirectionAzimuthAngle.size()));
    n = std::max(n, static_cast<int>(m_DirectionElevationAngle.size()));
    if(n <= 0) n = 1;
    m_directionCount = n;

    return true;
}

// ============================================================================
// UpdateCharacterizationFrequency
// ============================================================================

void COMM_AntennaPolarizationTx_Block::UpdateCharacterizationFrequency()
{
    double fc = 0.0;
    BufferReader* inputReader = GetInputPort("input");
    if(inputReader) {
        fc = inputReader->getCharacterizationFrequency();
    }
    m_inputFc = fc;

    // 传播到输出 bus
    auto* outV = GetOutputPort("output_V");
    if(outV) outV->setCharacterizationFrequency(fc);
    auto* outH = GetOutputPort("output_H");
    if(outH) outH->setCharacterizationFrequency(fc);
}

// ============================================================================
// DataStreamRun — 固定步长处理
// ============================================================================

bool COMM_AntennaPolarizationTx_Block::DataStreamRun()
{
    // 读取输入包络信号
    auto inputData = ReadInputData<EnvelopeSignal>(GetInputPortName(0));
    if(inputData.empty()) {
        return true;
    }

    const std::complex<double> x = inputData[0].complex();

    // 获取时间戳
    double timeNow = GetCurrentTime();

    // 读取 BeamAzimuth / BeamElevation（可选）
    bool hasBeamAz = false;
    double beamAzValue = 0.0;
    BufferReader* beamAzReader = GetInputPort("BeamAzimuth");
    if(beamAzReader && beamAzReader->IsConnected()) {
        auto data = ReadInputData<double>("BeamAzimuth");
        if(!data.empty()) { beamAzValue = data[0]; hasBeamAz = true; }
    }

    bool hasBeamEl = false;
    double beamElValue = 0.0;
    BufferReader* beamElReader = GetInputPort("BeamElevation");
    if(beamElReader && beamElReader->IsConnected()) {
        auto data = ReadInputData<double>("BeamElevation");
        if(!data.empty()) { beamElValue = data[0]; hasBeamEl = true; }
    }

    // 读取 DirectionAzimuth bus（可选）
    std::vector<double> dirAzData;
    BufferReader* dirAzReader = GetInputPort("DirectionAzimuth");
    if(dirAzReader && dirAzReader->IsConnected()) {
        dirAzData = ReadInputData<double>("DirectionAzimuth");
    }

    // 读取 DirectionElevation bus（可选）
    std::vector<double> dirElData;
    BufferReader* dirElReader = GetInputPort("DirectionElevation");
    if(dirElReader && dirElReader->IsConnected()) {
        dirElData = ReadInputData<double>("DirectionElevation");
    }

    // 计算波束角度
    double beamAzRad = 0.0;
    double beamElRad = 0.0;

    if(hasBeamAz) {
        beamAzRad = beamAzValue;
    } else if(m_BeamControlMode == 1) { // BeamSweep
        if(m_BeamScanPattern == 0) { // CircularScan
            beamAzRad = deg2rad(getCircularScanAzimuth(timeNow));
        } else if(m_BeamScanPattern == 1) { // BidirectionalSector
            beamAzRad = deg2rad(getSectorScanAzimuth(timeNow, true));
        } else if(m_BeamScanPattern == 2) { // UnidirectionalSector
            beamAzRad = deg2rad(getSectorScanAzimuth(timeNow, false));
        } else if(m_BeamScanPattern == 3) { // BidirectionalRaster
            double azDeg = 0.0, elDeg = 0.0;
            getRasterScanAngle(timeNow, true, azDeg, elDeg);
            beamAzRad = deg2rad(azDeg);
        } else if(m_BeamScanPattern == 4) { // UnidirectionalRaster
            double azDeg = 0.0, elDeg = 0.0;
            getRasterScanAngle(timeNow, false, azDeg, elDeg);
            beamAzRad = deg2rad(azDeg);
        } else {
            beamAzRad = deg2rad(m_BeamAzimuthAngle);
        }
    } else {
        beamAzRad = deg2rad(m_BeamAzimuthAngle);
    }

    if(hasBeamEl) {
        beamElRad = beamElValue;
    } else if(m_BeamControlMode == 1 && (m_BeamScanPattern == 3 || m_BeamScanPattern == 4)) {
        double azDeg = 0.0, elDeg = 0.0;
        getRasterScanAngle(timeNow, m_BeamScanPattern == 3, azDeg, elDeg);
        beamElRad = deg2rad(elDeg);
    } else if(m_BeamControlMode == 1) {
        beamElRad = deg2rad(m_ElevationAngle);
    } else {
        beamElRad = deg2rad(m_BeamElevationAngle);
    }

    // 确定输出通道数（运行时从 bus 连接数获取）
    Buffer* outVPort = GetOutputPort("output_V");
    Buffer* outHPort = GetOutputPort("output_H");
    const int outVChannels = outVPort ? static_cast<int>(outVPort->GetBusConnectionCount()) : 0;
    const int outHChannels = outHPort ? static_cast<int>(outHPort->GetBusConnectionCount()) : 0;
    int directionCount = std::max({m_directionCount, outVChannels, outHChannels});
    const int maxWritable = std::max(outVChannels, outHChannels);
    const int nRun = (maxWritable > 0) ? std::min(directionCount, maxWritable) : directionCount;

    // 初始化输出帧
    OutputFrame frame;
    frame.vChannels.resize(nRun, EnvelopeSignal(std::complex<double>(0.0, 0.0)));
    frame.hChannels.resize(nRun, EnvelopeSignal(std::complex<double>(0.0, 0.0)));

    // 对每个方向计算输出
    for(int ch = 0; ch < nRun; ++ch) {
        double directionAzRad = 0.0;
        double directionElRad = 0.0;

        // 获取方向角度
        if(ch < static_cast<int>(dirAzData.size())) {
            directionAzRad = dirAzData[static_cast<size_t>(ch)];
        } else {
            directionAzRad = deg2rad(getArrayValue(
                m_DirectionAzimuthAngle.empty() ? nullptr : m_DirectionAzimuthAngle.data(),
                static_cast<int>(m_DirectionAzimuthAngle.size()), ch, 0.0));
        }

        if(ch < static_cast<int>(dirElData.size())) {
            directionElRad = dirElData[static_cast<size_t>(ch)];
        } else {
            directionElRad = deg2rad(getArrayValue(
                m_DirectionElevationAngle.empty() ? nullptr : m_DirectionElevationAngle.data(),
                static_cast<int>(m_DirectionElevationAngle.size()), ch, 0.0));
        }

        const double relAzRad = normalizeRad(directionAzRad - beamAzRad);
        const double relElRad = normalizeRad(directionElRad - beamElRad);

        std::complex<double> Gtheta(0.0, 0.0);
        std::complex<double> Gphi(0.0, 0.0);

        if(m_PatternDataMode == 1) { // ImportActualPattern
            double thetaDeg = 90.0;
            double phiDeg = 0.0;
            azelToPatternThetaPhi(relAzRad, relElRad, thetaDeg, phiDeg);
            lookupImportedPolarizationGain(thetaDeg, phiDeg, Gtheta, Gphi);

            const double scaleTheta = getScaleValue(0);
            const double scalePhi = (static_cast<int>(m_ElementPatternFileScaleFactor.size()) >= 2)
                                    ? getScaleValue(1) : scaleTheta;
            Gtheta *= scaleTheta;
            Gphi *= scalePhi;

            if(m_ImportedGainMode == 1) { // NormalizeFileToPeakGain
                const double targetPeakAmplitude = linearAmplitudeFromDb(m_PeakGain_dBi);
                if(m_patternPeakAmplitude > 0.0) {
                    const double normalizationScale = targetPeakAmplitude / m_patternPeakAmplitude;
                    Gtheta *= normalizationScale;
                    Gphi *= normalizationScale;
                }
            }

            if(m_PolarizationType != 0) { // not PatternComponents
                const std::complex<double> scalarGain = combinePatternComponentsToScalar(Gtheta, Gphi);
                applyConfiguredPolarization(scalarGain, Gtheta, Gphi);
            }
        } else {
            const std::complex<double> scalarGain = calculateParametricScalarGain(relAzRad, relElRad);
            applyConfiguredPolarization(scalarGain, Gtheta, Gphi);
        }

        const std::complex<double> yV = x * Gtheta;
        const std::complex<double> yH = x * Gphi;

        frame.vChannels[static_cast<size_t>(ch)] = EnvelopeSignal(yV);
        frame.hChannels[static_cast<size_t>(ch)] = EnvelopeSignal(yH);
    }

    // 传播载波频率
    UpdateCharacterizationFrequency();

    // 写出到 bus 输出端口
    for(int ch = 0; ch < nRun; ++ch) {
        if(outVPort && ch < outVChannels) {
            outVPort->WriteEnvelopeDataToChannel(ch,
                std::vector<EnvelopeSignal>{frame.vChannels[static_cast<size_t>(ch)]}, m_inputFc);
        }
        if(outHPort && ch < outHChannels) {
            outHPort->WriteEnvelopeDataToChannel(ch,
                std::vector<EnvelopeSignal>{frame.hChannels[static_cast<size_t>(ch)]}, m_inputFc);
        }
    }

    return true;
}

// ============================================================================
// TimeDrivenRun — 变步长处理
// ============================================================================

bool COMM_AntennaPolarizationTx_Block::TimeDrivenRun()
{
    // 步骤1：读取输入数据
    auto inputData = ReadInputData<EnvelopeSignal>(GetInputPortName(0));

    // 步骤2：如果有输入数据，计算输出并推入队列
    if(!inputData.empty()) {
        const std::complex<double> x = inputData[0].complex();

        double timeNow = GetCurrentTime();

        // 读取可选端口
        bool hasBeamAz = false;
        double beamAzValue = 0.0;
        BufferReader* beamAzReader = GetInputPort("BeamAzimuth");
        if(beamAzReader && beamAzReader->IsConnected()) {
            auto data = ReadInputData<double>("BeamAzimuth");
            if(!data.empty()) { beamAzValue = data[0]; hasBeamAz = true; }
        }

        bool hasBeamEl = false;
        double beamElValue = 0.0;
        BufferReader* beamElReader = GetInputPort("BeamElevation");
        if(beamElReader && beamElReader->IsConnected()) {
            auto data = ReadInputData<double>("BeamElevation");
            if(!data.empty()) { beamElValue = data[0]; hasBeamEl = true; }
        }

        std::vector<double> dirAzData;
        BufferReader* dirAzReader = GetInputPort("DirectionAzimuth");
        if(dirAzReader && dirAzReader->IsConnected()) {
            dirAzData = ReadInputData<double>("DirectionAzimuth");
        }

        std::vector<double> dirElData;
        BufferReader* dirElReader = GetInputPort("DirectionElevation");
        if(dirElReader && dirElReader->IsConnected()) {
            dirElData = ReadInputData<double>("DirectionElevation");
        }

        // 计算波束角度
        double beamAzRad = 0.0, beamElRad = 0.0;
        if(hasBeamAz) { beamAzRad = beamAzValue; }
        else if(m_BeamControlMode == 1) {
            if(m_BeamScanPattern == 0) beamAzRad = deg2rad(getCircularScanAzimuth(timeNow));
            else if(m_BeamScanPattern == 1) beamAzRad = deg2rad(getSectorScanAzimuth(timeNow, true));
            else if(m_BeamScanPattern == 2) beamAzRad = deg2rad(getSectorScanAzimuth(timeNow, false));
            else if(m_BeamScanPattern == 3) { double a,e; getRasterScanAngle(timeNow, true, a, e); beamAzRad = deg2rad(a); }
            else if(m_BeamScanPattern == 4) { double a,e; getRasterScanAngle(timeNow, false, a, e); beamAzRad = deg2rad(a); }
            else beamAzRad = deg2rad(m_BeamAzimuthAngle);
        } else { beamAzRad = deg2rad(m_BeamAzimuthAngle); }

        if(hasBeamEl) { beamElRad = beamElValue; }
        else if(m_BeamControlMode == 1 && (m_BeamScanPattern == 3 || m_BeamScanPattern == 4)) {
            double a,e; getRasterScanAngle(timeNow, m_BeamScanPattern == 3, a, e); beamElRad = deg2rad(e);
        } else if(m_BeamControlMode == 1) { beamElRad = deg2rad(m_ElevationAngle); }
        else { beamElRad = deg2rad(m_BeamElevationAngle); }

        // 确定通道数（运行时从 bus 连接数获取）
        Buffer* outVPort = GetOutputPort("output_V");
        Buffer* outHPort = GetOutputPort("output_H");
        const int outVCh = outVPort ? static_cast<int>(outVPort->GetBusConnectionCount()) : 0;
        const int outHCh = outHPort ? static_cast<int>(outHPort->GetBusConnectionCount()) : 0;
        int dirCount = std::max({m_directionCount, outVCh, outHCh});
        const int maxW = std::max(outVCh, outHCh);
        const int nRun = (maxW > 0) ? std::min(dirCount, maxW) : dirCount;

        OutputFrame frame;
        frame.vChannels.resize(nRun, EnvelopeSignal(std::complex<double>(0.0, 0.0)));
        frame.hChannels.resize(nRun, EnvelopeSignal(std::complex<double>(0.0, 0.0)));

        for(int ch = 0; ch < nRun; ++ch) {
            double dAzRad = (ch < static_cast<int>(dirAzData.size()))
                ? dirAzData[static_cast<size_t>(ch)]
                : deg2rad(getArrayValue(m_DirectionAzimuthAngle.empty() ? nullptr : m_DirectionAzimuthAngle.data(),
                                        static_cast<int>(m_DirectionAzimuthAngle.size()), ch, 0.0));
            double dElRad = (ch < static_cast<int>(dirElData.size()))
                ? dirElData[static_cast<size_t>(ch)]
                : deg2rad(getArrayValue(m_DirectionElevationAngle.empty() ? nullptr : m_DirectionElevationAngle.data(),
                                        static_cast<int>(m_DirectionElevationAngle.size()), ch, 0.0));

            const double relAz = normalizeRad(dAzRad - beamAzRad);
            const double relEl = normalizeRad(dElRad - beamElRad);

            std::complex<double> Gt(0.0, 0.0), Gp(0.0, 0.0);

            if(m_PatternDataMode == 1) {
                double th = 90.0, ph = 0.0;
                azelToPatternThetaPhi(relAz, relEl, th, ph);
                lookupImportedPolarizationGain(th, ph, Gt, Gp);
                double sT = getScaleValue(0);
                double sP = (static_cast<int>(m_ElementPatternFileScaleFactor.size()) >= 2) ? getScaleValue(1) : sT;
                Gt *= sT; Gp *= sP;
                if(m_ImportedGainMode == 1 && m_patternPeakAmplitude > 0.0) {
                    double ns = linearAmplitudeFromDb(m_PeakGain_dBi) / m_patternPeakAmplitude;
                    Gt *= ns; Gp *= ns;
                }
                if(m_PolarizationType != 0) {
                    auto sg = combinePatternComponentsToScalar(Gt, Gp);
                    applyConfiguredPolarization(sg, Gt, Gp);
                }
            } else {
                auto sg = calculateParametricScalarGain(relAz, relEl);
                applyConfiguredPolarization(sg, Gt, Gp);
            }

            frame.vChannels[static_cast<size_t>(ch)] = EnvelopeSignal(x * Gt);
            frame.hChannels[static_cast<size_t>(ch)] = EnvelopeSignal(x * Gp);
        }

        m_outputQueue.push(std::move(frame));
        UpdateCharacterizationFrequency();
    }

    // 步骤3：从队列取一帧写出
    if(!m_outputQueue.empty()) {
        OutputFrame frame = std::move(m_outputQueue.front());
        m_outputQueue.pop();

        Buffer* outVPort = GetOutputPort("output_V");
        Buffer* outHPort = GetOutputPort("output_H");
        const int outVCh = outVPort ? static_cast<int>(outVPort->GetBusConnectionCount()) : 0;
        const int outHCh = outHPort ? static_cast<int>(outHPort->GetBusConnectionCount()) : 0;

        for(size_t i = 0; i < frame.vChannels.size(); ++i) {
            if(outVPort && static_cast<int>(i) < outVCh) {
                outVPort->WriteEnvelopeDataToChannel(static_cast<int>(i),
                    std::vector<EnvelopeSignal>{frame.vChannels[i]}, m_inputFc);
            }
        }
        for(size_t i = 0; i < frame.hChannels.size(); ++i) {
            if(outHPort && static_cast<int>(i) < outHCh) {
                outHPort->WriteEnvelopeDataToChannel(static_cast<int>(i),
                    std::vector<EnvelopeSignal>{frame.hChannels[i]}, m_inputFc);
            }
        }
    }

    return true;
}

// ============================================================================
// 配置校验
// ============================================================================

bool COMM_AntennaPolarizationTx_Block::validateConfiguration() const
{
    if(m_PatternDataMode == 0) {
        if(m_PolarizationType == 0) return false;
        if(m_ParametricPatternType != 0) {
            if(m_AzimuthHPBW <= 0.0 || m_ElevationHPBW <= 0.0) return false;
            if(m_ParametricPatternType == 1 && (m_AzimuthHPBW >= 180.0 || m_ElevationHPBW >= 180.0)) return false;
            if(m_MaxAttenuation_dB < 0.0) return false;
            if(m_VerticalSidelobeAttenuation_dB < 0.0) return false;
        }
    }
    if(m_XPD_dB < 0.0) return false;
    if(m_PolarizationType == 6) {
        double power = m_UserJonesHMagnitude * m_UserJonesHMagnitude + m_UserJonesVMagnitude * m_UserJonesVMagnitude;
        if(power <= 0.0) return false;
    }
    return true;
}

double COMM_AntennaPolarizationTx_Block::getArrayValue(const double* data, int size, int index, double defaultValue) const
{
    if(data == nullptr || size <= 0) return defaultValue;
    if(index < 0) return data[0];
    if(index < size) return data[index];
    return data[size - 1];
}

double COMM_AntennaPolarizationTx_Block::getScaleValue(int index) const
{
    if(m_ElementPatternFileScaleFactor.empty()) return 1.0;
    int sz = static_cast<int>(m_ElementPatternFileScaleFactor.size());
    if(index < 0) return m_ElementPatternFileScaleFactor[0];
    if(index < sz) return m_ElementPatternFileScaleFactor[static_cast<size_t>(index)];
    return m_ElementPatternFileScaleFactor[static_cast<size_t>(sz - 1)];
}

// ============================================================================
// 波束角度计算
// ============================================================================

double COMM_AntennaPolarizationTx_Block::getCircularScanAzimuth(double timeNow) const
{
    const double rate = m_ScanRate * 6.0;
    if(rate == 0.0) return 0.0;
    return wrap360(rate * timeNow);
}

double COMM_AntennaPolarizationTx_Block::getSectorScanAzimuth(double timeNow, bool bidirectional) const
{
    const double startDeg = m_SectorScanStartAngle;
    const double endDeg = m_SectorScanEndAngle;
    double width = endDeg - startDeg;
    if(std::fabs(width) < 1.0e-15) return startDeg;
    const double dir = (width >= 0.0) ? 1.0 : -1.0;
    width = std::fabs(width);
    const double rate = std::fabs(m_ScanRate * 6.0);
    if(rate <= 0.0) return startDeg;
    const double fwdTime = width / rate;
    if(bidirectional) {
        const double period = 2.0 * fwdTime;
        if(period <= 0.0) return startDeg;
        double t = std::fmod(timeNow, period);
        if(t < 0.0) t += period;
        if(t <= fwdTime) return startDeg + dir * rate * t;
        return endDeg - dir * rate * (t - fwdTime);
    }
    const double fb = std::max(0.0, m_FlybackTime);
    const double period = fwdTime + fb;
    if(period <= 0.0) return startDeg;
    double t = std::fmod(timeNow, period);
    if(t < 0.0) t += period;
    if(t <= fwdTime) return startDeg + dir * rate * t;
    if(fb > 0.0) { double k = (t - fwdTime) / fb; return endDeg + (startDeg - endDeg) * k; }
    return startDeg;
}

void COMM_AntennaPolarizationTx_Block::getRasterScanAngle(double timeNow, bool bidir, double& azDeg, double& elDeg) const
{
    const int barCount = std::max(1, m_NumberOfRasterBars + 1);
    const double sDeg = m_SectorScanStartAngle, eDeg = m_SectorScanEndAngle;
    double width = eDeg - sDeg;
    if(std::fabs(width) < 1.0e-15) { azDeg = sDeg; elDeg = m_ElevationAngle; return; }
    const double dir = (width >= 0.0) ? 1.0 : -1.0;
    width = std::fabs(width);
    const double rate = std::fabs(m_ScanRate * 6.0);
    if(rate <= 0.0) { azDeg = sDeg; elDeg = m_ElevationAngle; return; }
    const double scanT = width / rate;
    const double fb = std::max(0.0, m_FlybackTime);
    const double rowT = bidir ? scanT : (scanT + fb);
    const double period = rowT * barCount;
    if(period <= 0.0) { azDeg = sDeg; elDeg = m_ElevationAngle; return; }
    double t = std::fmod(timeNow, period);
    if(t < 0.0) t += period;
    int row = static_cast<int>(t / rowT);
    if(row >= barCount) row = barCount - 1;
    const double rowL = t - row * rowT;
    elDeg = m_ElevationAngle + row * m_RasterBarWidth;
    if(bidir) {
        if((row % 2) != 0) azDeg = eDeg - dir * rate * rowL;
        else azDeg = sDeg + dir * rate * rowL;
    } else if(rowL <= scanT) {
        azDeg = sDeg + dir * rate * rowL;
    } else if(fb > 0.0) {
        azDeg = eDeg + (sDeg - eDeg) * ((rowL - scanT) / fb);
    } else {
        azDeg = sDeg;
    }
}

// ============================================================================
// 方向图计算
// ============================================================================

double COMM_AntennaPolarizationTx_Block::calculateCosineExponent(double hpbwDeg) const
{
    const double hv = std::cos(deg2rad(0.5 * hpbwDeg));
    if(hv <= 0.0 || hv >= 1.0) return 0.0;
    return std::log(0.5) / std::log(hv);
}

double COMM_AntennaPolarizationTx_Block::calculateParametricAttenuationDb(double relAzRad, double relElRad) const
{
    if(m_ParametricPatternType == 0) return 0.0;
    const double azD = std::fabs(rad2deg(relAzRad));
    const double elD = std::fabs(rad2deg(relElRad));
    const double maxA = std::max(0.0, m_MaxAttenuation_dB);
    if(m_ParametricPatternType == 1) {
        double pA = 0.0, pE = 0.0;
        if(azD < 90.0) pA = std::pow(std::max(0.0, std::cos(deg2rad(azD))), calculateCosineExponent(m_AzimuthHPBW));
        if(elD < 90.0) pE = std::pow(std::max(0.0, std::cos(deg2rad(elD))), calculateCosineExponent(m_ElevationHPBW));
        double pw = pA * pE;
        if(pw <= 0.0) return maxA;
        return std::min(maxA, std::max(0.0, -10.0 * std::log10(pw)));
    }
    if(m_ParametricPatternType == 2) {
        return std::min(maxA, std::max(0.0,
            12.0 * (azD / m_AzimuthHPBW) * (azD / m_AzimuthHPBW) +
            12.0 * (elD / m_ElevationHPBW) * (elD / m_ElevationHPBW)));
    }
    double hA = std::min(12.0 * (azD / m_AzimuthHPBW) * (azD / m_AzimuthHPBW), maxA);
    double vA = std::min(12.0 * (elD / m_ElevationHPBW) * (elD / m_ElevationHPBW), std::max(0.0, m_VerticalSidelobeAttenuation_dB));
    return std::min(maxA, hA + vA);
}

std::complex<double> COMM_AntennaPolarizationTx_Block::calculateParametricScalarGain(double relAzRad, double relElRad) const
{
    return std::complex<double>(linearAmplitudeFromDb(m_PeakGain_dBi - calculateParametricAttenuationDb(relAzRad, relElRad)), 0.0);
}

// ============================================================================
// 极化计算
// ============================================================================

void COMM_AntennaPolarizationTx_Block::buildPolarizationJones(std::complex<double>& jV, std::complex<double>& jH) const
{
    jV = std::complex<double>(0.0, 0.0); jH = std::complex<double>(0.0, 0.0);
    switch(m_PolarizationType) {
    case 1: jH = {1.0, 0.0}; break;
    case 2: jV = {1.0, 0.0}; break;
    case 3: { double tr = deg2rad(m_PolarizationTiltAngle); jH = {std::cos(tr), 0.0}; jV = {std::sin(tr), 0.0}; break; }
    case 4: { double iv = 1.0/std::sqrt(2.0); jH = {iv, 0.0}; jV = {0.0, -iv}; break; }
    case 5: { double iv = 1.0/std::sqrt(2.0); jH = {iv, 0.0}; jV = {0.0, iv}; break; }
    case 6: jH = magPhaseToComplex(m_UserJonesHMagnitude, m_UserJonesHPhase, false, true);
            jV = magPhaseToComplex(m_UserJonesVMagnitude, m_UserJonesVPhase, false, true); break;
    default: jV = {1.0, 0.0}; break;
    }
    normalizeJones(jV, jH);
    double ca = std::pow(10.0, -std::max(0.0, m_XPD_dB) / 20.0);
    if(ca > 0.0) {
        auto oV = -std::conj(jH); auto oH = std::conj(jV);
        auto cc = magPhaseToComplex(ca, m_CrossPolarPhaseAngle, false, true);
        jV += cc * oV; jH += cc * oH;
        normalizeJones(jV, jH);
    }
}

void COMM_AntennaPolarizationTx_Block::applyConfiguredPolarization(
    const std::complex<double>& sg, std::complex<double>& Gt, std::complex<double>& Gp) const
{
    std::complex<double> jV, jH; buildPolarizationJones(jV, jH);
    Gt = sg * jV; Gp = sg * jH;
}

std::complex<double> COMM_AntennaPolarizationTx_Block::combinePatternComponentsToScalar(
    const std::complex<double>& Gt, const std::complex<double>& Gp) const
{
    double ta = std::sqrt(std::norm(Gt) + std::norm(Gp));
    if(ta <= 0.0) return {0.0, 0.0};
    auto dom = (std::abs(Gt) >= std::abs(Gp)) ? Gt : Gp;
    double da = std::abs(dom);
    if(da <= 0.0) return {ta, 0.0};
    return dom * (ta / da);
}

void COMM_AntennaPolarizationTx_Block::azelToPatternThetaPhi(double rAz, double rEl, double& th, double& ph) const
{
    th = clamp(90.0 - rad2deg(rEl), 0.0, 180.0);
    ph = wrap360(rad2deg(rAz));
}

// ============================================================================
// 实际方向图文件
// ============================================================================

void COMM_AntennaPolarizationTx_Block::clearPattern()
{ m_patternTable.clear(); m_patternLoaded = false; m_patternPeakAmplitude = 0.0; m_patternOpt = PatternFileOptions(); }

bool COMM_AntennaPolarizationTx_Block::loadPatternFile()
{
    clearPattern();
    if(m_TxAntennaPatternFileName.empty()) return false;
    std::ifstream fin(m_TxAntennaPatternFileName.c_str());
    if(!fin.good()) return false;
    bool inP = false, afterP = false, expP = false;
    std::string line;
    while(std::getline(fin, line)) {
        std::string t = trim(line); if(t.empty()) continue;
        std::string lo = lowerString(t);
        if(lo.find("begin_<parameters>") != std::string::npos || lo.find("begin_parameters") != std::string::npos)
        { inP = true; afterP = false; expP = true; continue; }
        if(lo.find("end_<parameters>") != std::string::npos || lo.find("end_parameters") != std::string::npos)
        { inP = false; afterP = true; continue; }
        if(inP) { parseParameterLine(t); continue; }
        if(expP && !afterP) continue;
        std::vector<double> nums; if(!parseNumericLine(t.c_str(), nums) || nums.size() < 4) continue;
        PatternPoint p; double th = nums[0], ph = nums[1];
        if(!m_patternOpt.directionInDegrees) { th = rad2deg(th); ph = rad2deg(ph); }
        p.thetaDeg = th; p.phiDeg = wrap360(ph);
        if(m_patternOpt.useMagPhase) {
            p.Gtheta = magPhaseToComplex(nums.size()>2?nums[2]:0.0, nums.size()>4?nums[4]:0.0, m_patternOpt.magnitudeInDb, m_patternOpt.phaseInDegrees);
            p.Gphi = magPhaseToComplex(nums.size()>3?nums[3]:nums.size()>2?nums[2]:0.0, nums.size()>5?nums[5]:0.0, m_patternOpt.magnitudeInDb, m_patternOpt.phaseInDegrees);
        } else {
            p.Gtheta = {nums.size()>2?nums[2]:1.0, nums.size()>3?nums[3]:0.0};
            p.Gphi = {nums.size()>4?nums[4]:nums.size()>2?nums[2]:1.0, nums.size()>5?nums[5]:0.0};
        }
        m_patternPeakAmplitude = std::max(m_patternPeakAmplitude, std::sqrt(std::norm(p.Gtheta)+std::norm(p.Gphi)));
        m_patternTable.push_back(p);
    }
    m_patternLoaded = !m_patternTable.empty(); return m_patternLoaded;
}

bool COMM_AntennaPolarizationTx_Block::parseParameterLine(const std::string& line)
{
    std::string lo = lowerString(line);
    size_t cp = lo.find("//"); if(cp != std::string::npos) lo = lo.substr(0, cp);
    lo = trim(lo); if(lo.empty()) return false;
    if(lo.find("mag_phase") != std::string::npos) { m_patternOpt.useMagPhase = true; return true; }
    if(lo.find("real_imag") != std::string::npos) { m_patternOpt.useMagPhase = false; return true; }
    if(lo.find("magnitude") != std::string::npos) {
        if(lo.find("db") != std::string::npos) m_patternOpt.magnitudeInDb = true;
        else if(lo.find("linear") != std::string::npos) m_patternOpt.magnitudeInDb = false; return true;
    }
    if(lo.find("direction") != std::string::npos) {
        if(lo.find("radian") != std::string::npos) m_patternOpt.directionInDegrees = false;
        else if(lo.find("degree") != std::string::npos) m_patternOpt.directionInDegrees = true; return true;
    }
    if(lo.find("phase") != std::string::npos) {
        if(lo.find("radian") != std::string::npos) m_patternOpt.phaseInDegrees = false;
        else if(lo.find("degree") != std::string::npos) m_patternOpt.phaseInDegrees = true; return true;
    }
    std::stringstream ss(lo); std::string key; double val; ss >> key >> val;
    if(ss.fail()) return false;
    if(key=="phi_min") { m_patternOpt.phiMin=val; return true; } if(key=="phi_max") { m_patternOpt.phiMax=val; return true; }
    if(key=="phi_inc") { m_patternOpt.phiInc=val; return true; } if(key=="theta_min") { m_patternOpt.thetaMin=val; return true; }
    if(key=="theta_max") { m_patternOpt.thetaMax=val; return true; } if(key=="theta_inc") { m_patternOpt.thetaInc=val; return true; }
    return false;
}

bool COMM_AntennaPolarizationTx_Block::parseNumericLine(const char* line, std::vector<double>& nums) const
{
    nums.clear(); if(!line) return false;
    std::string src = line; size_t cp = src.find("//"); if(cp != std::string::npos) src = src.substr(0, cp);
    std::string cl; for(char c : src) cl.push_back(((c>='0'&&c<='9')||c=='.'||c=='-'||c=='+'||c=='e'||c=='E')?c:' ');
    std::stringstream ss(cl); double v; while(ss >> v) nums.push_back(v);
    return !nums.empty();
}

void COMM_AntennaPolarizationTx_Block::lookupImportedPolarizationGain(
    double thD, double phD, std::complex<double>& Gt, std::complex<double>& Gp) const
{
    if(!m_patternLoaded || m_patternTable.empty()) { Gt = {0,0}; Gp = {0,0}; return; }
    int bi = 0; double bs = 1e300;
    for(size_t i = 0; i < m_patternTable.size(); ++i) {
        const auto& pt = m_patternTable[i];
        double td = thD - pt.thetaDeg, pd = angleDiffDeg(phD, pt.phiDeg);
        double sc = (m_ImportedPatternDimension == 0) ? pd*pd : td*td + pd*pd;
        if(sc < bs) { bs = sc; bi = (int)i; }
    }
    Gt = m_patternTable[bi].Gtheta; Gp = m_patternTable[bi].Gphi;
}

// ============================================================================
// 工具函数
// ============================================================================

std::string COMM_AntennaPolarizationTx_Block::trim(const std::string& s)
{ size_t b=0; while(b<s.size()&&std::isspace((unsigned char)s[b]))++b; size_t e=s.size(); while(e>b&&std::isspace((unsigned char)s[e-1]))--e; return s.substr(b,e-b); }

std::string COMM_AntennaPolarizationTx_Block::lowerString(const std::string& s)
{ std::string o=s; for(auto&c:o) c=(char)std::tolower((unsigned char)c); return o; }

double COMM_AntennaPolarizationTx_Block::deg2rad(double x) { return x * M_PI / 180.0; }
double COMM_AntennaPolarizationTx_Block::rad2deg(double x) { return x * 180.0 / M_PI; }
double COMM_AntennaPolarizationTx_Block::normalizeRad(double x) { while(x>M_PI)x-=2*M_PI; while(x<-M_PI)x+=2*M_PI; return x; }
double COMM_AntennaPolarizationTx_Block::wrap360(double x) { double y=std::fmod(x,360.0); if(y<0)y+=360.0; return y; }
double COMM_AntennaPolarizationTx_Block::clamp(double x, double lo, double hi) { if(x<lo)return lo; if(x>hi)return hi; return x; }
double COMM_AntennaPolarizationTx_Block::angleDiffDeg(double a, double b) { double d=a-b; while(d>180)d-=360; while(d<-180)d+=360; return d; }
double COMM_AntennaPolarizationTx_Block::linearAmplitudeFromDb(double g) { return std::pow(10.0, g/20.0); }

std::complex<double> COMM_AntennaPolarizationTx_Block::magPhaseToComplex(double mag, double ph, bool magDb, bool phDeg)
{ double a = magDb ? linearAmplitudeFromDb(mag) : mag; double p = phDeg ? deg2rad(ph) : ph; return {a*std::cos(p), a*std::sin(p)}; }

void COMM_AntennaPolarizationTx_Block::normalizeJones(std::complex<double>& jV, std::complex<double>& jH)
{ double pw = std::norm(jV)+std::norm(jH); if(pw<=0){jV={1,0};jH={0,0};return;} double s=1.0/std::sqrt(pw); jV*=s; jH*=s; }

int COMM_AntennaPolarizationTx_Block::ConvertStringToEnum(const std::string& value, int defaultValue)
{ std::string s = TrimCopy(value); if(s.empty()) return defaultValue; try { return std::stoi(s); } catch(...) {} return defaultValue; }
