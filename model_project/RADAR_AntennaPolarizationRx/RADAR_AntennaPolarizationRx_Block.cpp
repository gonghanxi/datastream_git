#include "RADAR_AntennaPolarizationRx_Block.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <complex>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================================================================
// 内部工具函数
// ============================================================================
namespace {

std::string TrimCopy(const std::string& value)
{
    std::string s = value;
    s.erase(s.begin(),
            std::find_if(s.begin(), s.end(),
                         [](unsigned char ch) { return !std::isspace(ch); }));
    s.erase(std::find_if(s.rbegin(), s.rend(),
                         [](unsigned char ch) { return !std::isspace(ch); }).base(),
            s.end());
    return s;
}

std::string ToLowerCopy(const std::string& value)
{
    std::string s = value;
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return s;
}

} // namespace

// ============================================================================
// 构造函数
// ============================================================================

RADAR_AntennaPolarizationRx_Block::RADAR_AntennaPolarizationRx_Block(const std::string& name)
    : Block(name)
    , m_patternLoaded(false)
{
}

// ============================================================================
// 默认参数
// ============================================================================

void RADAR_AntennaPolarizationRx_Block::SetDefaultParameters()
{
    m_RadarWorkMode                        = SelectedRadarWorkMode::Tracking;
    m_ElementPatternFileType               = SelectedElementPatternFileType::EMPro;
    m_ElementPatternFileScaleFactor        = nullptr;
    m_ElementPatternFileScaleFactor_Size   = 0;
    m_ElementPatternFileScaleFactor_data.clear();
    m_UserDefinedAntennaPattern            = SelectedUserDefinedAntennaPattern::UserDefine3D;
    m_RxAntennaPatternFileName1.clear();
    m_AntennaScanPattern                   = SelectedAntennaScanPattern::CircularScan;
    m_ScanRate                             = 15.0;
    m_ElevationAngle                       = 0.0;
    m_SectorScanStartAngle                 = 0.0;
    m_SectorScanEndAngle                   = 0.0;
    m_FlybackTime                          = 0.0;
    m_NumberOfRasterBars                   = 0;
    m_RasterBarWidth                       = 5.0;
    m_TargetAzimuthAngle                   = nullptr;
    m_TargetAzimuthAngle_Size              = 0;
    m_TargetAzimuthAngle_data.clear();
    m_TargetElevationAngle                 = nullptr;
    m_TargetElevationAngle_Size            = 0;
    m_TargetElevationAngle_data.clear();
    m_BeamAzimuthAngle                     = 0.0;
    m_BeamElevationAngle                   = 0.0;
    m_patternTable.clear();
    m_patternLoaded                        = false;
}

// ============================================================================
// 参数同步到算法实例
// ============================================================================

void RADAR_AntennaPolarizationRx_Block::SetParameters()
{
    // 将 vector 数据地址同步到指针变量和长度变量
    m_ElementPatternFileScaleFactor      = m_ElementPatternFileScaleFactor_data.empty()
                                               ? nullptr
                                               : m_ElementPatternFileScaleFactor_data.data();
    m_ElementPatternFileScaleFactor_Size = static_cast<int>(m_ElementPatternFileScaleFactor_data.size());

    m_TargetAzimuthAngle      = m_TargetAzimuthAngle_data.empty()
                                    ? nullptr
                                    : m_TargetAzimuthAngle_data.data();
    m_TargetAzimuthAngle_Size = static_cast<int>(m_TargetAzimuthAngle_data.size());

    m_TargetElevationAngle      = m_TargetElevationAngle_data.empty()
                                      ? nullptr
                                      : m_TargetElevationAngle_data.data();
    m_TargetElevationAngle_Size = static_cast<int>(m_TargetElevationAngle_data.size());

    if (!m_algo) { return; }

    m_algo->RadarWorkMode                      = m_RadarWorkMode;
    m_algo->ElementPatternFileType             = m_ElementPatternFileType;
    m_algo->ElementPatternFileScaleFactor      = m_ElementPatternFileScaleFactor;
    m_algo->ElementPatternFileScaleFactor_Size = m_ElementPatternFileScaleFactor_Size;
    m_algo->UserDefinedAntennaPattern          = m_UserDefinedAntennaPattern;
    m_algo->RxAntennaPatternFileName1          =
        m_RxAntennaPatternFileName1.empty()
            ? nullptr
            : const_cast<char*>(m_RxAntennaPatternFileName1.c_str());
    m_algo->AntennaScanPattern                 = m_AntennaScanPattern;
    m_algo->ScanRate                           = m_ScanRate;
    m_algo->ElevationAngle                     = m_ElevationAngle;
    m_algo->SectorScanStartAngle               = m_SectorScanStartAngle;
    m_algo->SectorScanEndAngle                 = m_SectorScanEndAngle;
    m_algo->FlybackTime                        = m_FlybackTime;
    m_algo->NumberOfRasterBars                 = m_NumberOfRasterBars;
    m_algo->RasterBarWidth                     = m_RasterBarWidth;
    m_algo->TargetAzimuthAngle                 = m_TargetAzimuthAngle;
    m_algo->TargetAzimuthAngle_Size            = m_TargetAzimuthAngle_Size;
    m_algo->TargetElevationAngle               = m_TargetElevationAngle;
    m_algo->TargetElevationAngle_Size          = m_TargetElevationAngle_Size;
    m_algo->BeamAzimuthAngle                   = m_BeamAzimuthAngle;
    m_algo->BeamElevationAngle                 = m_BeamElevationAngle;
}

// ============================================================================
// Setup / Run / Initialize
// ============================================================================

bool RADAR_AntennaPolarizationRx_Block::Setup()
{
    Block::Setup();
    while (!m_azQueue.empty())      m_azQueue.pop();
    while (!m_elQueue.empty())      m_elQueue.pop();
    while (!m_beamAzQueue.empty())  m_beamAzQueue.pop();
    while (!m_beamElQueue.empty())  m_beamElQueue.pop();
    while (!m_inputVQueue.empty())  m_inputVQueue.pop();
    while (!m_inputHQueue.empty())  m_inputHQueue.pop();
    while (!m_outputVQueue.empty()) m_outputVQueue.pop();
    while (!m_outputHQueue.empty()) m_outputHQueue.pop();
    return true;
}

bool RADAR_AntennaPolarizationRx_Block::Run()
{
    if (IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool RADAR_AntennaPolarizationRx_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_algo = std::make_unique<RADAR_AntennaPolarizationRx>();
    SetDefaultParameters();

    // ---- 读取参数 ----
    try { m_RadarWorkMode = ConvertStringToRadarWorkMode(
              getParameter("RadarWorkMode").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'RadarWorkMode', using default value."); }

    try { m_ElementPatternFileType = ConvertStringToElementPatternFileType(
              getParameter("ElementPatternFileType").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'ElementPatternFileType', using default value."); }

    try {
        std::string scaleStr = getParameter("ElementPatternFileScaleFactor").Value;
        parseArrayString(scaleStr, m_ElementPatternFileScaleFactor_data);
    } catch (...) { LOG_WARN("Failed to parse parameter 'ElementPatternFileScaleFactor', using default value."); }

    try { m_UserDefinedAntennaPattern = ConvertStringToUserDefinedAntennaPattern(
              getParameter("UserDefinedAntennaPattern").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'UserDefinedAntennaPattern', using default value."); }

    try { m_RxAntennaPatternFileName1 =
              TrimCopy(getParameter("RxAntennaPatternFileName1").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'RxAntennaPatternFileName1', using default value."); }

    try { m_AntennaScanPattern = ConvertStringToAntennaScanPattern(
              getParameter("AntennaScanPattern").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'AntennaScanPattern', using default value."); }

    try { m_ScanRate           = std::stod(getParameter("ScanRate").Value);           } catch (...) { LOG_WARN("Failed to parse parameter 'ScanRate', using default value."); }
    try { m_ElevationAngle     = std::stod(getParameter("ElevationAngle").Value);     } catch (...) { LOG_WARN("Failed to parse parameter 'ElevationAngle', using default value."); }
    try { m_SectorScanStartAngle = std::stod(getParameter("SectorScanStartAngle").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'SectorScanStartAngle', using default value."); }
    try { m_SectorScanEndAngle   = std::stod(getParameter("SectorScanEndAngle").Value);   } catch (...) { LOG_WARN("Failed to parse parameter 'SectorScanEndAngle', using default value."); }
    try { m_FlybackTime        = std::stod(getParameter("FlybackTime").Value);        } catch (...) { LOG_WARN("Failed to parse parameter 'FlybackTime', using default value."); }
    try { m_NumberOfRasterBars = std::stoi(getParameter("NumberOfRasterBars").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'NumberOfRasterBars', using default value."); }
    try { m_RasterBarWidth     = std::stod(getParameter("RasterBarWidth").Value);     } catch (...) { LOG_WARN("Failed to parse parameter 'RasterBarWidth', using default value."); }
    try {
        std::string azStr = getParameter("TargetAzimuthAngle").Value;
        parseArrayString(azStr, m_TargetAzimuthAngle_data);
    } catch (...) { LOG_WARN("Failed to parse parameter 'TargetAzimuthAngle', using default value."); }
    try {
        std::string elStr = getParameter("TargetElevationAngle").Value;
        parseArrayString(elStr, m_TargetElevationAngle_data);
    } catch (...) { LOG_WARN("Failed to parse parameter 'TargetElevationAngle', using default value."); }
    try { m_BeamAzimuthAngle   = std::stod(getParameter("BeamAzimuthAngle").Value);   } catch (...) { LOG_WARN("Failed to parse parameter 'BeamAzimuthAngle', using default value."); }
    try { m_BeamElevationAngle = std::stod(getParameter("BeamElevationAngle").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'BeamElevationAngle', using default value."); }

    // 加载天线方向图文件（由 Block 层自行维护 pattern table）
    SetParameters();
    loadPatternFile();

    // ---- 注册端口 ----
    // 输入
    AddInputPort("TargetAzimuth",  m_algo->TargetAzimuth,  1, Block::DataType::DOUBLE_BUS);
    AddInputPort("TargetElevation",m_algo->TargetElevation,1, Block::DataType::DOUBLE_BUS);
    AddInputPort("BeamAzimuth",    m_algo->BeamAzimuth,    1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddInputPort("BeamElevation",  m_algo->BeamElevation,  1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddInputPort("input_V",        m_algo->input_V,        1, Block::DataType::ENVELOPE_BUS);
    AddInputPort("input_H",        m_algo->input_H,        1, Block::DataType::ENVELOPE_BUS);

    // 输出
    AddOutputPort("output_V", m_algo->output_V, 1, Block::DataType::ENVELOPE_SIGNAL);
    AddOutputPort("output_H", m_algo->output_H, 1, Block::DataType::ENVELOPE_SIGNAL);

    return true;
}

// ============================================================================
// DataStreamRun：核心运行逻辑
// ============================================================================

bool RADAR_AntennaPolarizationRx_Block::DataStreamRun()
{
    SetParameters();

    // 读取各 bus 输入
    auto inputV_data = ReadInputData<EnvelopeSignal>(GetInputPortName(4));
    auto inputH_data = ReadInputData<EnvelopeSignal>(GetInputPortName(5));

    auto azData  = ReadInputData<double>(GetInputPortName(0));
    auto elData  = ReadInputData<double>(GetInputPortName(1));
    auto bAzData = ReadInputData<double>(GetInputPortName(2));
    auto bElData = ReadInputData<double>(GetInputPortName(3));

    const bool hasInputV     = !inputV_data.empty();
    const bool hasInputH     = !inputH_data.empty();
    const bool hasTargetAz   = !azData.empty();
    const bool hasTargetEl   = !elData.empty();
    const bool hasBeamAz     = !bAzData.empty();
    const bool hasBeamEl     = !bElData.empty();

    // 确定目标数量
    int nTarget = 0;
    if (hasInputH) { nTarget = std::max(nTarget, static_cast<int>(inputH_data.size())); }
    if (hasInputV) { nTarget = std::max(nTarget, static_cast<int>(inputV_data.size())); }
    if (hasTargetAz) { nTarget = std::max(nTarget, static_cast<int>(azData.size())); }
    if (hasTargetEl) { nTarget = std::max(nTarget, static_cast<int>(elData.size())); }
        nTarget = std::max(nTarget, m_TargetAzimuthAngle_Size);
    nTarget = std::max(nTarget, m_TargetElevationAngle_Size);
    if (nTarget <= 0) { nTarget = 1; }

    // 波束角度
    double beamAzRad = 0.0;
    double beamElRad = 0.0;

    if (hasBeamAz) {
        beamAzRad = bAzData[0];
    }
    if (hasBeamEl) {
        beamElRad = bElData[0];
    }
    if (!hasBeamAz || !hasBeamEl) {
        double azTmp = 0.0, elTmp = 0.0;
        getBeamAngle(0.0, azTmp, elTmp);
        if (!hasBeamAz) { beamAzRad = azTmp; }
        if (!hasBeamEl) { beamElRad = elTmp; }
    }

    std::complex<double> yH(0.0, 0.0);
    std::complex<double> yV(0.0, 0.0);

    for (int ch = 0; ch < nTarget; ++ch) {
        // 读取输入信号
        std::complex<double> xH(0.0, 0.0);
        std::complex<double> xV(0.0, 0.0);

        if (hasInputH && ch < static_cast<int>(inputH_data.size())) {
            xH = inputH_data[ch].complex();
        }
        if (hasInputV && ch < static_cast<int>(inputV_data.size())) {
            xV = inputV_data[ch].complex();
        }

        // 目标方位角
        double targetAzRad = 0.0;
        if (hasTargetAz && ch < static_cast<int>(azData.size())) {
            targetAzRad = azData[ch];
        } else {
            targetAzRad = deg2rad(getArrayValue(
                m_TargetAzimuthAngle, m_TargetAzimuthAngle_Size, ch, 0.0));
        }

        // 目标俯仰角
        double targetElRad = 0.0;
        if (hasTargetEl && ch < static_cast<int>(elData.size())) {
            targetElRad = elData[ch];
        } else {
            targetElRad = deg2rad(getArrayValue(
                m_TargetElevationAngle, m_TargetElevationAngle_Size, ch, 0.0));
        }

        // 相对角度（度）
        const double relAzDeg = rad2deg(normalizeRad(targetAzRad - beamAzRad));
        const double relElDeg = rad2deg(normalizeRad(targetElRad - beamElRad));

        // 查表极化矩阵
        std::complex<double> GHH, GHV, GVH, GVV;
        lookupPolarizationMatrix(relAzDeg, relElDeg, GHH, GHV, GVH, GVV);

        yH += GHH * xH + GHV * xV;
        yV += GVH * xH + GVV * xV;
    }

    WriteOutputData(GetOutputPortName(0), std::vector<EnvelopeSignal>{ EnvelopeSignal(yV) });
    WriteOutputData(GetOutputPortName(1), std::vector<EnvelopeSignal>{ EnvelopeSignal(yH) });

    if(m_algo){
        m_algo->Advance();
    }
    return true;
}

bool RADAR_AntennaPolarizationRx_Block::TimeDrivenRun()
{

        auto azData  = ReadInputData<double>(GetInputPortName(0));
        auto elData  = ReadInputData<double>(GetInputPortName(1));
        auto bAzData = ReadInputData<double>(GetInputPortName(2));
        auto bElData = ReadInputData<double>(GetInputPortName(3));
        auto inputV  = ReadInputData<EnvelopeSignal>(GetInputPortName(4));
        auto inputH  = ReadInputData<EnvelopeSignal>(GetInputPortName(5));

        for (auto& v : azData)  m_azQueue.push(v);
        for (auto& v : elData)  m_elQueue.push(v);
        for (auto& v : bAzData) m_beamAzQueue.push(v);
        for (auto& v : bElData) m_beamElQueue.push(v);
        for (auto& v : inputV)  m_inputVQueue.push(v);
        for (auto& v : inputH)  m_inputHQueue.push(v);

    // ② inputV 和 inputH 同时非空 → 逐对 pop 处理
    if (!m_inputVQueue.empty() && !m_inputHQueue.empty())
    {
        EnvelopeSignal sigV = m_inputVQueue.front(); m_inputVQueue.pop();
        EnvelopeSignal sigH = m_inputHQueue.front(); m_inputHQueue.pop();
        std::complex<double> xV = sigV.complex();
        std::complex<double> xH = sigH.complex();

        double targetAzRad = 0.0;
        if (!m_azQueue.empty()) { targetAzRad = m_azQueue.front(); m_azQueue.pop(); }

        double targetElRad = 0.0;
        if (!m_elQueue.empty()) { targetElRad = m_elQueue.front(); m_elQueue.pop(); }

        double beamAzRad = 0.0;
        double beamElRad = 0.0;
        bool hasBeamAz = !m_beamAzQueue.empty();
        bool hasBeamEl = !m_beamElQueue.empty();
        if (hasBeamAz) { beamAzRad = m_beamAzQueue.front(); m_beamAzQueue.pop(); }
        if (hasBeamEl) { beamElRad = m_beamElQueue.front(); m_beamElQueue.pop(); }
        if (!hasBeamAz || !hasBeamEl) {
            double azTmp = 0.0, elTmp = 0.0;
            getBeamAngle(0.0, azTmp, elTmp);
            if (!hasBeamAz) { beamAzRad = azTmp; }
            if (!hasBeamEl) { beamElRad = elTmp; }
        }

        const double relAzDeg = rad2deg(normalizeRad(targetAzRad - beamAzRad));
        const double relElDeg = rad2deg(normalizeRad(targetElRad - beamElRad));

        std::complex<double> GHH, GHV, GVH, GVV;
        lookupPolarizationMatrix(relAzDeg, relElDeg, GHH, GHV, GVH, GVV);

        std::complex<double> yH = GHH * xH + GHV * xV;
        std::complex<double> yV = GVH * xH + GVV * xV;

        m_outputVQueue.push(EnvelopeSignal(yV));
        m_outputHQueue.push(EnvelopeSignal(yH));
    }

    // ③ 出队写入，输出后清空全部 6 路输入队列
    bool wroteOutput = false;
    if (!m_outputVQueue.empty()) {
        EnvelopeSignal outV = m_outputVQueue.front(); m_outputVQueue.pop();
        WriteOutputData(GetOutputPortName(0), std::vector<EnvelopeSignal>{outV});
        wroteOutput = true;
    }
    if (!m_outputHQueue.empty()) {
        EnvelopeSignal outH = m_outputHQueue.front(); m_outputHQueue.pop();
        WriteOutputData(GetOutputPortName(1), std::vector<EnvelopeSignal>{outH});
        wroteOutput = true;
    }
    if (wroteOutput) {
        m_azQueue      = std::queue<double>();
        m_elQueue      = std::queue<double>();
        m_beamAzQueue  = std::queue<double>();
        m_beamElQueue  = std::queue<double>();
        m_inputVQueue  = std::queue<EnvelopeSignal>();
        m_inputHQueue  = std::queue<EnvelopeSignal>();
    }

    return true;
}

// ============================================================================
// 天线方向图文件加载（Block 层独立维护）
// ============================================================================

void RADAR_AntennaPolarizationRx_Block::loadPatternFile()
{
    m_patternTable.clear();
    m_patternLoaded = false;

    if (m_RxAntennaPatternFileName1.empty()) { return; }

    std::ifstream fin(m_RxAntennaPatternFileName1.c_str());
    if (!fin.good()) { return; }

    std::string line;
    while (std::getline(fin, line)) {
        // 将非数字字符替换为空格
        std::string cleaned;
        for (char c : line) {
            if ((c >= '0' && c <= '9') || c == '.' || c == '-' ||
                c == '+' || c == 'e'  || c == 'E') {
                cleaned.push_back(c);
            } else {
                cleaned.push_back(' ');
            }
        }

        std::stringstream ss(cleaned);
        std::vector<double> nums;
        double v = 0.0;
        while (ss >> v) { nums.push_back(v); }
        if (nums.size() < 3) { continue; }

        PatternPoint p;
        p.azDeg = nums[0];
        p.elDeg = (nums.size() >= 4) ? nums[1] : 0.0;

        p.GHH = std::complex<double>(1.0, 0.0);
        p.GHV = std::complex<double>(0.0, 0.0);
        p.GVH = std::complex<double>(0.0, 0.0);
        p.GVV = std::complex<double>(1.0, 0.0);

        if (nums.size() >= 10) {
            // az el GHH_re GHH_im GHV_re GHV_im GVH_re GVH_im GVV_re GVV_im
            p.GHH = std::complex<double>(nums[2], nums[3]);
            p.GHV = std::complex<double>(nums[4], nums[5]);
            p.GVH = std::complex<double>(nums[6], nums[7]);
            p.GVV = std::complex<double>(nums[8], nums[9]);
        } else if (nums.size() >= 6) {
            // az el H_dB H_phaseDeg V_dB V_phaseDeg
            p.GHH = dbPhaseToComplex(nums[2], nums[3]);
            p.GVV = dbPhaseToComplex(nums[4], nums[5]);
        } else if (nums.size() >= 4) {
            // az el H_dB V_dB
            p.GHH = dbPhaseToComplex(nums[2], 0.0);
            p.GVV = dbPhaseToComplex(nums[3], 0.0);
        } else {
            // az el gain_dB
            p.GHH = dbPhaseToComplex(nums[2], 0.0);
            p.GVV = p.GHH;
        }

        m_patternTable.push_back(p);
    }

    m_patternLoaded = !m_patternTable.empty();
}

// ============================================================================
// 极化矩阵查表
// ============================================================================

void RADAR_AntennaPolarizationRx_Block::lookupPolarizationMatrix(
    double relAzDeg, double relElDeg,
    std::complex<double>& GHH, std::complex<double>& GHV,
    std::complex<double>& GVH, std::complex<double>& GVV) const
{
    // 未加载方向图时使用单位矩阵（透传）
    if (!m_patternLoaded || m_patternTable.empty()) {
        GHH = std::complex<double>(1.0, 0.0);
        GHV = std::complex<double>(0.0, 0.0);
        GVH = std::complex<double>(0.0, 0.0);
        GVV = std::complex<double>(1.0, 0.0);
        return;
    }

    // 最近邻查找
    int    bestIndex = 0;
    double bestScore = 1.0e300;

    for (size_t i = 0; i < m_patternTable.size(); ++i) {
        const PatternPoint& pt = m_patternTable[i];
        const double da = angleDiffDeg(relAzDeg, pt.azDeg);
        double de = 0.0;
        if (m_UserDefinedAntennaPattern == SelectedUserDefinedAntennaPattern::UserDefine3D) {
            de = relElDeg - pt.elDeg;
        }
        const double score = da * da + de * de;
        if (score < bestScore) {
            bestScore = score;
            bestIndex = static_cast<int>(i);
        }
    }

    const PatternPoint& best = m_patternTable[static_cast<size_t>(bestIndex)];
    GHH = best.GHH;
    GHV = best.GHV;
    GVH = best.GVH;
    GVV = best.GVV;

    // 应用缩放因子
    const double s0 = getScaleValue(0);
    if (static_cast<int>(m_ElementPatternFileScaleFactor_Size) >= 4) {
        GHH *= getScaleValue(0);
        GHV *= getScaleValue(1);
        GVH *= getScaleValue(2);
        GVV *= getScaleValue(3);
    } else {
        GHH *= s0;
        GHV *= s0;
        GVH *= s0;
        GVV *= s0;
    }
}

// ============================================================================
// 波束角度计算
// ============================================================================

void RADAR_AntennaPolarizationRx_Block::getBeamAngle(double timeNow,
                                                       double& beamAzRad,
                                                       double& beamElRad)
{
    // 方位角
    if (m_RadarWorkMode == SelectedRadarWorkMode::Search) {
        switch (m_AntennaScanPattern) {
        case SelectedAntennaScanPattern::CircularScan:
            beamAzRad = deg2rad(getCircularScanAzimuth(timeNow));
            break;
        case SelectedAntennaScanPattern::BidirectionalSector:
            beamAzRad = deg2rad(getSectorScanAzimuth(timeNow, true));
            break;
        case SelectedAntennaScanPattern::UnidirectionalSector:
            beamAzRad = deg2rad(getSectorScanAzimuth(timeNow, false));
            break;
        case SelectedAntennaScanPattern::BidirectionalRaster: {
            double azDeg = 0.0, elDeg = 0.0;
            getRasterScanAngle(timeNow, true, azDeg, elDeg);
            beamAzRad = deg2rad(azDeg);
            break;
        }
        case SelectedAntennaScanPattern::UnidirectionalRaster: {
            double azDeg = 0.0, elDeg = 0.0;
            getRasterScanAngle(timeNow, false, azDeg, elDeg);
            beamAzRad = deg2rad(azDeg);
            break;
        }
        default:
            beamAzRad = deg2rad(m_BeamAzimuthAngle);
            break;
        }
    } else {
        beamAzRad = deg2rad(m_BeamAzimuthAngle);
    }

    // 俯仰角
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

double RADAR_AntennaPolarizationRx_Block::getCircularScanAzimuth(double timeNow) const
{
    const double rateDegPerSec = m_ScanRate * 6.0;
    if (rateDegPerSec == 0.0) { return 0.0; }
    return wrapTo360(rateDegPerSec * timeNow);
}

double RADAR_AntennaPolarizationRx_Block::getSectorScanAzimuth(double timeNow,
                                                                 bool bidirectional) const
{
    const double startDeg = m_SectorScanStartAngle;
    const double endDeg   = m_SectorScanEndAngle;
    double width = endDeg - startDeg;
    if (std::fabs(width) < 1.0e-15) { return startDeg; }

    const double dir  = (width >= 0.0) ? 1.0 : -1.0;
    width = std::fabs(width);
    const double rate = std::fabs(m_ScanRate * 6.0);
    if (rate <= 0.0) { return startDeg; }

    const double forwardTime = width / rate;

    if (bidirectional) {
        const double period = 2.0 * forwardTime;
        if (period <= 0.0) { return startDeg; }
        double t = std::fmod(timeNow, period);
        if (t < 0.0) { t += period; }
        if (t <= forwardTime) { return startDeg + dir * rate * t; }
        return endDeg - dir * rate * (t - forwardTime);
    } else {
        const double fb = std::max(0.0, m_FlybackTime);
        const double period = forwardTime + fb;
        if (period <= 0.0) { return startDeg; }
        double t = std::fmod(timeNow, period);
        if (t < 0.0) { t += period; }
        if (t <= forwardTime) { return startDeg + dir * rate * t; }
        if (fb > 0.0) {
            const double k = (t - forwardTime) / fb;
            return endDeg + (startDeg - endDeg) * k;
        }
        return startDeg;
    }
}

void RADAR_AntennaPolarizationRx_Block::getRasterScanAngle(double timeNow,
                                                             bool bidirectional,
                                                             double& azDeg,
                                                             double& elDeg) const
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

    const double dir  = (width >= 0.0) ? 1.0 : -1.0;
    width = std::fabs(width);
    const double rate = std::fabs(m_ScanRate * 6.0);
    if (rate <= 0.0) {
        azDeg = startDeg;
        elDeg = m_ElevationAngle;
        return;
    }

    const double scanTime = width / rate;
    const double fb       = std::max(0.0, m_FlybackTime);
    const double rowTime  = bidirectional ? scanTime : (scanTime + fb);
    const double period   = rowTime * barCount;
    if (period <= 0.0) {
        azDeg = startDeg;
        elDeg = m_ElevationAngle;
        return;
    }

    double t = std::fmod(timeNow, period);
    if (t < 0.0) { t += period; }

    int row = static_cast<int>(t / rowTime);
    if (row >= barCount) { row = barCount - 1; }

    const double rowLocal = t - row * rowTime;
    elDeg = m_ElevationAngle + row * m_RasterBarWidth;

    if (bidirectional) {
        const bool reverse = (row % 2) != 0;
        azDeg = reverse
            ? (endDeg - dir * rate * rowLocal)
            : (startDeg + dir * rate * rowLocal);
    } else {
        if (rowLocal <= scanTime) {
            azDeg = startDeg + dir * rate * rowLocal;
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
// 辅助函数
// ============================================================================

double RADAR_AntennaPolarizationRx_Block::getArrayValue(const double* data,
                                                          int size,
                                                          int index,
                                                          double defaultValue) const
{
    if (data == nullptr || size <= 0) { return defaultValue; }
    if (index < 0)     { return data[0]; }
    if (index < size)  { return data[index]; }
    return data[size - 1];
}

double RADAR_AntennaPolarizationRx_Block::getScaleValue(int index) const
{
    if (m_ElementPatternFileScaleFactor == nullptr ||
        m_ElementPatternFileScaleFactor_Size <= 0) { return 1.0; }
    if (index < 0) { return m_ElementPatternFileScaleFactor[0]; }
    if (index < m_ElementPatternFileScaleFactor_Size) {
        return m_ElementPatternFileScaleFactor[index];
    }
    return m_ElementPatternFileScaleFactor[m_ElementPatternFileScaleFactor_Size - 1];
}

double RADAR_AntennaPolarizationRx_Block::deg2rad(double x)
{
    return x * M_PI / 180.0;
}

double RADAR_AntennaPolarizationRx_Block::rad2deg(double x)
{
    return x * 180.0 / M_PI;
}

double RADAR_AntennaPolarizationRx_Block::normalizeRad(double x)
{
    while (x > M_PI)  { x -= 2.0 * M_PI; }
    while (x < -M_PI) { x += 2.0 * M_PI; }
    return x;
}

double RADAR_AntennaPolarizationRx_Block::wrapTo360(double x)
{
    double y = std::fmod(x, 360.0);
    if (y < 0.0) { y += 360.0; }
    return y;
}

double RADAR_AntennaPolarizationRx_Block::angleDiffDeg(double a, double b)
{
    double d = a - b;
    while (d > 180.0)  { d -= 360.0; }
    while (d < -180.0) { d += 360.0; }
    return d;
}

std::complex<double> RADAR_AntennaPolarizationRx_Block::dbPhaseToComplex(double db,
                                                                           double phaseDeg)
{
    const double amp = std::pow(10.0, db / 20.0);
    const double ph  = deg2rad(phaseDeg);
    return std::complex<double>(amp * std::cos(ph), amp * std::sin(ph));
}

// ============================================================================
// 数组字符串解析  "[1, 2, 3]" → vector<double>
// ============================================================================

bool RADAR_AntennaPolarizationRx_Block::parseArrayString(const std::string& arrayStr,
                                                           std::vector<double>& outArray)
{
    outArray.clear();

    std::string str = TrimCopy(arrayStr);
    if (str.empty()) { return false; }

    // 支持带括号格式 [1,2,3] 或不带括号的纯数字列表
    if (str.front() == '[' && str.back() == ']') {
        str = str.substr(1, str.size() - 2);
        str = TrimCopy(str);
    }

    if (str.empty()) { return true; }

    std::stringstream ss(str);
    std::string item;
    while (std::getline(ss, item, ',')) {
        item = TrimCopy(item);
        if (item.empty()) { continue; }
        try {
            outArray.push_back(std::stod(item));
        } catch (...) {
            return false;
        }
    }

    return true;
}

// ============================================================================
// 枚举字符串转换
// ============================================================================

RADAR_AntennaPolarizationRx_Block::SelectedRadarWorkMode
RADAR_AntennaPolarizationRx_Block::ConvertStringToRadarWorkMode(const std::string& value)
{
    const std::string s = ToLowerCopy(TrimCopy(value));
    if (s == "tracking" || s == "0") { return SelectedRadarWorkMode::Tracking; }
    if (s == "search"   || s == "1") { return SelectedRadarWorkMode::Search;   }
    return SelectedRadarWorkMode::Tracking;
}

RADAR_AntennaPolarizationRx_Block::SelectedElementPatternFileType
RADAR_AntennaPolarizationRx_Block::ConvertStringToElementPatternFileType(const std::string& value)
{
    const std::string s = ToLowerCopy(TrimCopy(value));
    if (s == "empro" || s == "0") { return SelectedElementPatternFileType::EMPro; }
    if (s == "hfss"  || s == "1") { return SelectedElementPatternFileType::HFSS;  }
    if (s == "cst"   || s == "2") { return SelectedElementPatternFileType::CST;   }
    return SelectedElementPatternFileType::EMPro;
}

RADAR_AntennaPolarizationRx_Block::SelectedUserDefinedAntennaPattern
RADAR_AntennaPolarizationRx_Block::ConvertStringToUserDefinedAntennaPattern(const std::string& value)
{
    const std::string s = ToLowerCopy(TrimCopy(value));
    if (s == "userdefine2d" || s == "0") { return SelectedUserDefinedAntennaPattern::UserDefine2D; }
    if (s == "userdefine3d" || s == "1") { return SelectedUserDefinedAntennaPattern::UserDefine3D; }
    return SelectedUserDefinedAntennaPattern::UserDefine3D;
}

RADAR_AntennaPolarizationRx_Block::SelectedAntennaScanPattern
RADAR_AntennaPolarizationRx_Block::ConvertStringToAntennaScanPattern(const std::string& value)
{
    const std::string s = ToLowerCopy(TrimCopy(value));
    if (s == "circular"              || s == "0") { return SelectedAntennaScanPattern::CircularScan;        }
    if (s == "bidirectional sector"  || s == "1") { return SelectedAntennaScanPattern::BidirectionalSector; }
    if (s == "unidirectional sector" || s == "2") { return SelectedAntennaScanPattern::UnidirectionalSector;}
    if (s == "bidirectional raster"  || s == "3") { return SelectedAntennaScanPattern::BidirectionalRaster; }
    if (s == "unidirectional raster" || s == "4") { return SelectedAntennaScanPattern::UnidirectionalRaster;}
    return SelectedAntennaScanPattern::CircularScan;
}
