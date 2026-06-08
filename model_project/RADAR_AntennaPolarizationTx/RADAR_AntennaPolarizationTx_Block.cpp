#include "RADAR_AntennaPolarizationTx_Block.h"

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

RADAR_AntennaPolarizationTx_Block::RADAR_AntennaPolarizationTx_Block(const std::string& name)
    : Block(name)
    , m_patternLoaded(false)
{
}

// ============================================================================
// 默认参数
// ============================================================================

void RADAR_AntennaPolarizationTx_Block::SetDefaultParameters()
{
    m_RadarWorkMode                        = SelectedRadarWorkMode::Tracking;
    m_ElementPatternFileType               = SelectedElementPatternFileType::EMPro;
    m_ElementPatternFileScaleFactor        = nullptr;
    m_ElementPatternFileScaleFactor_Size   = 0;
    m_ElementPatternFileScaleFactor_data.clear();
    m_UserDefinedAntennaPattern            = SelectedUserDefinedAntennaPattern::UserDefine3D;
    m_TxAntennaPatternFileName1.clear();
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
    m_patternOpt                           = PatternFileOptions();
    m_patternLoaded                        = false;
}

// ============================================================================
// 参数同步到算法实例
// ============================================================================

void RADAR_AntennaPolarizationTx_Block::SetParameters()
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
    m_algo->TxAntennaPatternFileName1          =
        m_TxAntennaPatternFileName1.empty()
            ? nullptr
            : const_cast<char*>(m_TxAntennaPatternFileName1.c_str());
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

bool RADAR_AntennaPolarizationTx_Block::Setup()
{
    Block::Setup();
    while (!m_azQueue.empty())      m_azQueue.pop();
    while (!m_elQueue.empty())      m_elQueue.pop();
    while (!m_beamAzQueue.empty())  m_beamAzQueue.pop();
    while (!m_beamElQueue.empty())  m_beamElQueue.pop();
    while (!m_inputQueue.empty())   m_inputQueue.pop();
    while (!m_outputVQueue.empty()) m_outputVQueue.pop();
    while (!m_outputHQueue.empty()) m_outputHQueue.pop();
    return true;
}

bool RADAR_AntennaPolarizationTx_Block::Run()
{
    if (IsVariableStepMode()) { return TimeDrivenRun(); }
    return DataStreamRun();
}

bool RADAR_AntennaPolarizationTx_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_algo = std::make_unique<RADAR_AntennaPolarizationTx>();
    SetDefaultParameters();

    // ---- 读取参数 ----
    try { m_RadarWorkMode = ConvertStringToRadarWorkMode(
              getParameter("RadarWorkMode").Value); } catch (...) {}

    try { m_ElementPatternFileType = ConvertStringToElementPatternFileType(
              getParameter("ElementPatternFileType").Value); } catch (...) {}

    try {
        std::string scaleStr = getParameter("ElementPatternFileScaleFactor").Value;
        parseArrayString(scaleStr, m_ElementPatternFileScaleFactor_data);
    } catch (...) {}

    try { m_UserDefinedAntennaPattern = ConvertStringToUserDefinedAntennaPattern(
              getParameter("UserDefinedAntennaPattern").Value); } catch (...) {}

    try { m_TxAntennaPatternFileName1 =
              TrimCopy(getParameter("TxAntennaPatternFileName1").Value); } catch (...) {}

    try { m_AntennaScanPattern = ConvertStringToAntennaScanPattern(
              getParameter("AntennaScanPattern").Value); } catch (...) {}

    try { m_ScanRate             = std::stod(getParameter("ScanRate").Value);             } catch (...) {}
    try { m_ElevationAngle       = std::stod(getParameter("ElevationAngle").Value);       } catch (...) {}
    try { m_SectorScanStartAngle = std::stod(getParameter("SectorScanStartAngle").Value); } catch (...) {}
    try { m_SectorScanEndAngle   = std::stod(getParameter("SectorScanEndAngle").Value);   } catch (...) {}
    try { m_FlybackTime          = std::stod(getParameter("FlybackTime").Value);          } catch (...) {}
    try { m_NumberOfRasterBars   = std::stoi(getParameter("NumberOfRasterBars").Value);   } catch (...) {}
    try { m_RasterBarWidth       = std::stod(getParameter("RasterBarWidth").Value);       } catch (...) {}

    try {
        std::string azStr = getParameter("TargetAzimuthAngle").Value;
        parseArrayString(azStr, m_TargetAzimuthAngle_data);
    } catch (...) {}

    try {
        std::string elStr = getParameter("TargetElevationAngle").Value;
        parseArrayString(elStr, m_TargetElevationAngle_data);
    } catch (...) {}

    try { m_BeamAzimuthAngle   = std::stod(getParameter("BeamAzimuthAngle").Value);   } catch (...) {}
    try { m_BeamElevationAngle = std::stod(getParameter("BeamElevationAngle").Value); } catch (...) {}

    SetParameters();
    loadPatternFile();

    // ---- 注册端口 ----
    // 输入
    AddInputPort("TargetAzimuth",  m_algo->TargetAzimuth,  1, Block::DataType::DOUBLE_BUS);
    AddInputPort("TargetElevation",m_algo->TargetElevation,1, Block::DataType::DOUBLE_BUS);
    AddInputPort("BeamAzimuth",    m_algo->BeamAzimuth,    1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddInputPort("BeamElevation",  m_algo->BeamElevation,  1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddInputPort("input",          m_algo->input,          1, Block::DataType::ENVELOPE_SIGNAL);

    // 输出（多路 Bus）
    AddOutputPort("output_V", m_algo->output_V, 1, Block::DataType::ENVELOPE_BUS);
    AddOutputPort("output_H", m_algo->output_H, 1, Block::DataType::ENVELOPE_BUS);

    return true;
}

// ============================================================================
// DataStreamRun：核心运行逻辑
// ============================================================================

bool RADAR_AntennaPolarizationTx_Block::DataStreamRun()
{
    SetParameters();

    // 读取单路 envelope 输入
    auto inputData = ReadInputData<EnvelopeSignal>(GetInputPortName(4));
    if (inputData.empty()) { return false; }

    const std::complex<double> x = inputData[0].complex();

    // 读取角度端口
    auto azData  = ReadInputData<double>(GetInputPortName(0));
    auto elData  = ReadInputData<double>(GetInputPortName(1));
    auto bAzData = ReadInputData<double>(GetInputPortName(2));
    auto bElData = ReadInputData<double>(GetInputPortName(3));

    const bool hasTargetAz = !azData.empty();
    const bool hasTargetEl = !elData.empty();
    const bool hasBeamAz   = !bAzData.empty();
    const bool hasBeamEl   = !bElData.empty();

    // 确定目标数量
    int nTarget = 0;
    if (hasTargetAz) { nTarget = std::max(nTarget, static_cast<int>(azData.size())); }
    if (hasTargetEl) { nTarget = std::max(nTarget, static_cast<int>(elData.size())); }
    nTarget = std::max(nTarget, m_TargetAzimuthAngle_Size);
    nTarget = std::max(nTarget, m_TargetElevationAngle_Size);
    if (nTarget <= 0) { nTarget = 1; }

    // 波束角度
    double beamAzRad = 0.0;
    double beamElRad = 0.0;

    if (hasBeamAz) { beamAzRad = bAzData[0]; }
    if (hasBeamEl) { beamElRad = bElData[0]; }
    if (!hasBeamAz || !hasBeamEl) {
        double azTmp = 0.0, elTmp = 0.0;
        getBeamAngle(0.0, azTmp, elTmp);
        if (!hasBeamAz) { beamAzRad = azTmp; }
        if (!hasBeamEl) { beamElRad = elTmp; }
    }

    // 缩放因子
    const double scaleTheta = getScaleValue(0);
    const double scalePhi   = (m_ElementPatternFileScaleFactor_Size >= 2)
                                  ? getScaleValue(1)
                                  : scaleTheta;

    // 每个目标输出一路
    std::vector<EnvelopeSignal> outV_vec;
    std::vector<EnvelopeSignal> outH_vec;
    outV_vec.reserve(static_cast<size_t>(nTarget));
    outH_vec.reserve(static_cast<size_t>(nTarget));

    for (int ch = 0; ch < nTarget; ++ch) {
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

        // 相对角度（弧度）
        const double relAzRad = normalizeRad(targetAzRad - beamAzRad);
        const double relElRad = normalizeRad(targetElRad - beamElRad);

        // az/el -> theta/phi 坐标转换
        double thetaDeg = 90.0;
        double phiDeg   = 0.0;
        azelToPatternThetaPhi(relAzRad, relElRad, thetaDeg, phiDeg);

        // 查表极化增益
        std::complex<double> Gtheta(1.0, 0.0);
        std::complex<double> Gphi(1.0, 0.0);
        lookupPolarizationGain(thetaDeg, phiDeg, Gtheta, Gphi);

        Gtheta *= scaleTheta;
        Gphi   *= scalePhi;

        outV_vec.push_back(EnvelopeSignal(x * Gtheta));  // theta -> V
        outH_vec.push_back(EnvelopeSignal(x * Gphi));    // phi   -> H
    }

    WriteOutputData(GetOutputPortName(0), outV_vec);
    WriteOutputData(GetOutputPortName(1), outH_vec);

    if (m_algo) {
        m_algo->Advance();
    }
    return true;
}

// ============================================================================
// TimeDrivenRun：变步长逐点处理
// ============================================================================

bool RADAR_AntennaPolarizationTx_Block::TimeDrivenRun()
{
    SetParameters();

    // ① 累积全部 5 路输入到各自队列
    {
        auto azData    = ReadInputData<double>(GetInputPortName(0));
        auto elData    = ReadInputData<double>(GetInputPortName(1));
        auto bAzData   = ReadInputData<double>(GetInputPortName(2));
        auto bElData   = ReadInputData<double>(GetInputPortName(3));
        auto inputData = ReadInputData<EnvelopeSignal>(GetInputPortName(4));

        for (auto& v : azData)    m_azQueue.push(v);
        for (auto& v : elData)    m_elQueue.push(v);
        for (auto& v : bAzData)   m_beamAzQueue.push(v);
        for (auto& v : bElData)   m_beamElQueue.push(v);
        for (auto& v : inputData) m_inputQueue.push(v);
    }

    // ② input 非空 → 逐点 pop 处理
    const double scaleTheta = getScaleValue(0);
    const double scalePhi   = (m_ElementPatternFileScaleFactor_Size >= 2)
                                  ? getScaleValue(1)
                                  : scaleTheta;

    if (!m_inputQueue.empty())
    {
        EnvelopeSignal sig = m_inputQueue.front(); m_inputQueue.pop();
        std::complex<double> x = sig.complex();

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

        const double relAzRad = normalizeRad(targetAzRad - beamAzRad);
        const double relElRad = normalizeRad(targetElRad - beamElRad);

        double thetaDeg = 90.0;
        double phiDeg   = 0.0;
        azelToPatternThetaPhi(relAzRad, relElRad, thetaDeg, phiDeg);

        std::complex<double> Gtheta(1.0, 0.0);
        std::complex<double> Gphi(1.0, 0.0);
        lookupPolarizationGain(thetaDeg, phiDeg, Gtheta, Gphi);

        Gtheta *= scaleTheta;
        Gphi   *= scalePhi;

        m_outputVQueue.push(EnvelopeSignal(x * Gtheta));
        m_outputHQueue.push(EnvelopeSignal(x * Gphi));
    }

    // ③ 出队写入，输出后清空全部 5 路输入队列
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
        m_azQueue     = std::queue<double>();
        m_elQueue     = std::queue<double>();
        m_beamAzQueue = std::queue<double>();
        m_beamElQueue = std::queue<double>();
        m_inputQueue  = std::queue<EnvelopeSignal>();
    }

    return true;
}

// ============================================================================
// 天线方向图文件加载（Block 层独立维护，支持 EMPro/HFSS/CST 格式）
// ============================================================================

void RADAR_AntennaPolarizationTx_Block::loadPatternFile()
{
    m_patternTable.clear();
    m_patternOpt    = PatternFileOptions();
    m_patternLoaded = false;

    if (m_TxAntennaPatternFileName1.empty()) { return; }

    std::ifstream fin(m_TxAntennaPatternFileName1.c_str());
    if (!fin.good()) { return; }

    bool inParameterSection    = false;
    bool afterParameterSection = false;

    std::string line;
    while (std::getline(fin, line)) {
        const std::string t   = TrimCopy(line);
        if (t.empty()) { continue; }

        const std::string low = ToLowerCopy(t);

        if (low.find("begin_<parameters>") != std::string::npos ||
            low.find("begin_parameters")   != std::string::npos) {
            inParameterSection    = true;
            afterParameterSection = false;
            continue;
        }

        if (low.find("end_<parameters>") != std::string::npos ||
            low.find("end_parameters")   != std::string::npos) {
            inParameterSection    = false;
            afterParameterSection = true;
            continue;
        }

        if (inParameterSection) {
            parseParameterLine(t);
            continue;
        }

        if (!afterParameterSection) {
            // 尝试解析为数字行；非数字行跳过
            std::vector<double> tryNums;
            if (!parseNumericLine(t.c_str(), tryNums)) { continue; }
        }

        std::vector<double> nums;
        if (!parseNumericLine(t.c_str(), nums)) { continue; }
        if (nums.size() < 4) { continue; }

        PatternPoint p;

        double theta = nums[0];
        double phi   = nums[1];

        if (!m_patternOpt.directionInDegrees) {
            theta = rad2deg(theta);
            phi   = rad2deg(phi);
        }

        p.thetaDeg = theta;
        p.phiDeg   = wrapTo360(phi);

        if (m_patternOpt.useMagPhase) {
            // theta_angle, phi_angle, theta_gain, phi_gain, [theta_phase, phi_phase]
            const double thetaGain  = nums.size() > 2 ? nums[2] : 0.0;
            const double phiGain    = nums.size() > 3 ? nums[3] : thetaGain;
            const double thetaPhase = nums.size() > 4 ? nums[4] : 0.0;
            const double phiPhase   = nums.size() > 5 ? nums[5] : 0.0;

            p.Gtheta = magPhaseToComplex(thetaGain,  thetaPhase,
                                         m_patternOpt.magnitudeInDb,
                                         m_patternOpt.phaseInDegrees);
            p.Gphi   = magPhaseToComplex(phiGain,    phiPhase,
                                         m_patternOpt.magnitudeInDb,
                                         m_patternOpt.phaseInDegrees);
        } else {
            // theta_angle, phi_angle, re_theta, im_theta, re_phi, im_phi
            const double tRe = nums.size() > 2 ? nums[2] : 1.0;
            const double tIm = nums.size() > 3 ? nums[3] : 0.0;
            const double pRe = nums.size() > 4 ? nums[4] : tRe;
            const double pIm = nums.size() > 5 ? nums[5] : tIm;

            p.Gtheta = std::complex<double>(tRe, tIm);
            p.Gphi   = std::complex<double>(pRe, pIm);
        }

        m_patternTable.push_back(p);
    }

    m_patternLoaded = !m_patternTable.empty();
}

// ============================================================================
// 方向图文件参数行解析
// ============================================================================

bool RADAR_AntennaPolarizationTx_Block::parseParameterLine(const std::string& line)
{
    std::string low = ToLowerCopy(line);

    // 去除注释
    const size_t cpos = low.find("//");
    if (cpos != std::string::npos) { low = low.substr(0, cpos); }
    low = TrimCopy(low);
    if (low.empty()) { return false; }

    if (low.find("mag_phase")  != std::string::npos) { m_patternOpt.useMagPhase = true;  return true; }
    if (low.find("real_imag")  != std::string::npos) { m_patternOpt.useMagPhase = false; return true; }

    if (low.find("magnitude") != std::string::npos) {
        if (low.find("db")     != std::string::npos) { m_patternOpt.magnitudeInDb = true;  }
        if (low.find("linear") != std::string::npos) { m_patternOpt.magnitudeInDb = false; }
        return true;
    }

    if (low.find("direction") != std::string::npos) {
        if (low.find("radian") != std::string::npos) { m_patternOpt.directionInDegrees = false; }
        if (low.find("degree") != std::string::npos) { m_patternOpt.directionInDegrees = true;  }
        return true;
    }

    if (low.find("phase") != std::string::npos) {
        if (low.find("radian") != std::string::npos) { m_patternOpt.phaseInDegrees = false; }
        if (low.find("degree") != std::string::npos) { m_patternOpt.phaseInDegrees = true;  }
        return true;
    }

    std::stringstream ss(low);
    std::string key;
    double value = 0.0;
    ss >> key >> value;

    if (!ss.fail()) {
        if (key == "phi_min")   { m_patternOpt.phiMin   = value; return true; }
        if (key == "phi_max")   { m_patternOpt.phiMax   = value; return true; }
        if (key == "phi_inc")   { m_patternOpt.phiInc   = value; return true; }
        if (key == "theta_min") { m_patternOpt.thetaMin = value; return true; }
        if (key == "theta_max") { m_patternOpt.thetaMax = value; return true; }
        if (key == "theta_inc") { m_patternOpt.thetaInc = value; return true; }
    }

    return false;
}

// ============================================================================
// 数字行解析（去除注释后提取所有浮点数）
// ============================================================================

bool RADAR_AntennaPolarizationTx_Block::parseNumericLine(const char* line,
                                                          std::vector<double>& nums) const
{
    nums.clear();
    if (line == nullptr) { return false; }

    std::string src = line;

    // 去除 // 注释
    const size_t cpos = src.find("//");
    if (cpos != std::string::npos) { src = src.substr(0, cpos); }

    std::string cleaned;
    for (size_t i = 0; i < src.size(); ++i) {
        const char c = src[i];
        if ((c >= '0' && c <= '9') || c == '.' || c == '-' ||
            c == '+' || c == 'e'  || c == 'E') {
            cleaned.push_back(c);
        } else {
            cleaned.push_back(' ');
        }
    }

    std::stringstream ss(cleaned);
    double v = 0.0;
    while (ss >> v) { nums.push_back(v); }

    return !nums.empty();
}

// ============================================================================
// 极化增益查表（最近邻，theta/phi 坐标系）
// ============================================================================

void RADAR_AntennaPolarizationTx_Block::lookupPolarizationGain(
    double thetaDeg, double phiDeg,
    std::complex<double>& Gtheta,
    std::complex<double>& Gphi) const
{
    if (!m_patternLoaded || m_patternTable.empty()) {
        Gtheta = std::complex<double>(1.0, 0.0);
        Gphi   = std::complex<double>(1.0, 0.0);
        return;
    }

    int    bestIndex = 0;
    double bestScore = 1.0e300;

    for (size_t i = 0; i < m_patternTable.size(); ++i) {
        const PatternPoint& pt = m_patternTable[i];
        const double dt = thetaDeg - pt.thetaDeg;
        const double dp = angleDiffDeg(phiDeg, pt.phiDeg);

        double score = 0.0;
        if (m_UserDefinedAntennaPattern == SelectedUserDefinedAntennaPattern::UserDefine2D) {
            score = dp * dp;                // 2D 仅按 phi 匹配
        } else {
            score = dt * dt + dp * dp;     // 3D theta+phi 联合匹配
        }

        if (score < bestScore) {
            bestScore = score;
            bestIndex = static_cast<int>(i);
        }
    }

    const PatternPoint& best = m_patternTable[static_cast<size_t>(bestIndex)];
    Gtheta = best.Gtheta;
    Gphi   = best.Gphi;
}

// ============================================================================
// az/el -> theta/phi 坐标转换
// ============================================================================

void RADAR_AntennaPolarizationTx_Block::azelToPatternThetaPhi(double relAzRad,
                                                               double relElRad,
                                                               double& thetaDeg,
                                                               double& phiDeg) const
{
    const double azDeg = rad2deg(relAzRad);
    const double elDeg = rad2deg(relElRad);
    thetaDeg = clampValue(90.0 - elDeg, 0.0, 180.0);
    phiDeg   = wrapTo360(azDeg);
}

// ============================================================================
// 波束角度计算
// ============================================================================

void RADAR_AntennaPolarizationTx_Block::getBeamAngle(double timeNow,
                                                      double& beamAzRad,
                                                      double& beamElRad)
{
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

double RADAR_AntennaPolarizationTx_Block::getCircularScanAzimuth(double timeNow) const
{
    const double rateDegPerSec = m_ScanRate * 6.0;
    if (rateDegPerSec == 0.0) { return 0.0; }
    return wrapTo360(rateDegPerSec * timeNow);
}

double RADAR_AntennaPolarizationTx_Block::getSectorScanAzimuth(double timeNow,
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

void RADAR_AntennaPolarizationTx_Block::getRasterScanAngle(double timeNow,
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

double RADAR_AntennaPolarizationTx_Block::getArrayValue(const double* data,
                                                         int size,
                                                         int index,
                                                         double defaultValue) const
{
    if (data == nullptr || size <= 0) { return defaultValue; }
    if (index < 0)    { return data[0]; }
    if (index < size) { return data[index]; }
    return data[size - 1];
}

double RADAR_AntennaPolarizationTx_Block::getScaleValue(int index) const
{
    if (m_ElementPatternFileScaleFactor == nullptr ||
        m_ElementPatternFileScaleFactor_Size <= 0) { return 1.0; }
    if (index < 0) { return m_ElementPatternFileScaleFactor[0]; }
    if (index < m_ElementPatternFileScaleFactor_Size) {
        return m_ElementPatternFileScaleFactor[index];
    }
    return m_ElementPatternFileScaleFactor[m_ElementPatternFileScaleFactor_Size - 1];
}

double RADAR_AntennaPolarizationTx_Block::deg2rad(double x)
{
    return x * M_PI / 180.0;
}

double RADAR_AntennaPolarizationTx_Block::rad2deg(double x)
{
    return x * 180.0 / M_PI;
}

double RADAR_AntennaPolarizationTx_Block::normalizeRad(double x)
{
    while (x > M_PI)  { x -= 2.0 * M_PI; }
    while (x < -M_PI) { x += 2.0 * M_PI; }
    return x;
}

double RADAR_AntennaPolarizationTx_Block::wrapTo360(double x)
{
    double y = std::fmod(x, 360.0);
    if (y < 0.0) { y += 360.0; }
    return y;
}

double RADAR_AntennaPolarizationTx_Block::clampValue(double x, double lo, double hi)
{
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}

double RADAR_AntennaPolarizationTx_Block::angleDiffDeg(double a, double b)
{
    double d = a - b;
    while (d > 180.0)  { d -= 360.0; }
    while (d < -180.0) { d += 360.0; }
    return d;
}

std::complex<double> RADAR_AntennaPolarizationTx_Block::magPhaseToComplex(
    double magnitude, double phase, bool magnitudeInDb, bool phaseInDegrees)
{
    const double amp      = magnitudeInDb ? std::pow(10.0, magnitude / 20.0) : magnitude;
    const double phaseRad = phaseInDegrees ? deg2rad(phase) : phase;
    return std::complex<double>(amp * std::cos(phaseRad), amp * std::sin(phaseRad));
}

// ============================================================================
// 数组字符串解析  "[1, 2, 3]" → vector<double>
// ============================================================================

bool RADAR_AntennaPolarizationTx_Block::parseArrayString(const std::string& arrayStr,
                                                          std::vector<double>& outArray)
{
    outArray.clear();

    std::string str = TrimCopy(arrayStr);
    if (str.empty()) { return false; }

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

RADAR_AntennaPolarizationTx_Block::SelectedRadarWorkMode
RADAR_AntennaPolarizationTx_Block::ConvertStringToRadarWorkMode(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "tracking" || lower == "0") { return SelectedRadarWorkMode::Tracking; }
    if (lower == "search"   || lower == "1") { return SelectedRadarWorkMode::Search;   }
    return SelectedRadarWorkMode::Tracking;
}

RADAR_AntennaPolarizationTx_Block::SelectedElementPatternFileType
RADAR_AntennaPolarizationTx_Block::ConvertStringToElementPatternFileType(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "empro" || lower == "0") { return SelectedElementPatternFileType::EMPro; }
    if (lower == "hfss"  || lower == "1") { return SelectedElementPatternFileType::HFSS;  }
    if (lower == "cst"   || lower == "2") { return SelectedElementPatternFileType::CST;   }
    return SelectedElementPatternFileType::EMPro;
}

RADAR_AntennaPolarizationTx_Block::SelectedUserDefinedAntennaPattern
RADAR_AntennaPolarizationTx_Block::ConvertStringToUserDefinedAntennaPattern(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "userdefine2d" || lower == "0") { return SelectedUserDefinedAntennaPattern::UserDefine2D; }
    if (lower == "userdefine3d" || lower == "1") { return SelectedUserDefinedAntennaPattern::UserDefine3D; }
    return SelectedUserDefinedAntennaPattern::UserDefine3D;
}

RADAR_AntennaPolarizationTx_Block::SelectedAntennaScanPattern
RADAR_AntennaPolarizationTx_Block::ConvertStringToAntennaScanPattern(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "circular"              || lower == "0") { return SelectedAntennaScanPattern::CircularScan;         }
    if (lower == "bidirectionalsector"  || lower == "1") { return SelectedAntennaScanPattern::BidirectionalSector;  }
    if (lower == "unidirectionalsector" || lower == "2") { return SelectedAntennaScanPattern::UnidirectionalSector; }
    if (lower == "bidirectionalraster"  || lower == "3") { return SelectedAntennaScanPattern::BidirectionalRaster;  }
    if (lower == "unidirectionalraster" || lower == "4") { return SelectedAntennaScanPattern::UnidirectionalRaster; }
    return SelectedAntennaScanPattern::CircularScan;
}
