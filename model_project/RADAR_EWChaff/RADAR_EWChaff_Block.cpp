#include "RADAR_EWChaff_Block.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <sstream>
#include <string>

// ============================================================================
// 常数
// ============================================================================

static const double kPi         = 3.14159265358979323846;
static const double kSpeedOfLight = 299792458.0;
static const double kGoldenAngle  = 2.39996322972865332223;

// ============================================================================
// 匿名命名空间 — 工具函数
// ============================================================================

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

std::vector<double> ParseStringToDoubleVector(const std::string& value)
{
    std::vector<double> result;
    std::string s = value;
    size_t start = s.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return result;
    size_t end = s.find_last_not_of(" \t\n\r");
    s = s.substr(start, end - start + 1);
    if (s.empty() || s.front() != '[' || s.back() != ']') return result;
    s = s.substr(1, s.size() - 2);
    std::string token;
    for (size_t i = 0; i <= s.size(); ++i) {
        if (i == s.size() || s[i] == ',' || s[i] == ' ' || s[i] == '\t') {
            if (!token.empty()) {
                try { result.push_back(std::stod(token)); } catch (...) {}
                token.clear();
            }
        } else {
            token += s[i];
        }
    }
    if (!token.empty()) {
        try { result.push_back(std::stod(token)); } catch (...) {}
    }
    return result;
}

} // anonymous namespace

// ============================================================================
// 构造函数
// ============================================================================

RADAR_EWChaff_Block::RADAR_EWChaff_Block(const std::string& name)
    : Block(name)
    , m_Chaff_Mode(RADAR_EWChaff::ReleasedCloud)
    , m_Cloud_Model(RADAR_EWChaff::MultiCell)
    , m_RCS_Model(RADAR_EWChaff::ResonantDipoleApprox)
    , m_RCS_OutputUnit(RADAR_EWChaff::Linear_m2)
    , m_Cell_RCS_Distribution(RADAR_EWChaff::GaussianCenter_Distribution)
    , m_NumberOfChaffCell(16)
    , m_Fall_Speed(0.0)
    , m_VelocityDecayTime(0.5)
    , m_Cloud_Lifetime(30.0)
    , m_CarrierFreq(10e9)
    , m_DipoleLength(0.015)
    , m_DipoleLengthSpread(0.003)
    , m_NumberOfDipoles(100000.0)
    , m_ReferenceDipoleCount(100000.0)
    , m_TotalRCS_Reference(1000.0)
    , m_RCS_GrowthTime(0.2)
    , m_RCS_DecayTime(20.0)
    , m_RCS_Floor(0.0)
    , m_GaussianWeightSigma(0.65)
    , m_TimeStep(1e-3)
    , m_sampleIndex(0)
    , m_released(false)
    , m_releaseSampleIndex(0)
    , m_userPathIndex(0)
{
    m_releasePosLatched = m_make_vec(0.0, 0.0, 0.0);
    m_releaseVelLatched = m_make_vec(0.0, 0.0, 0.0);
}

// ============================================================================
// SetDefaultParameters
// ============================================================================

void RADAR_EWChaff_Block::SetDefaultParameters()
{
    m_Chaff_Mode           = RADAR_EWChaff::ReleasedCloud;
    m_Cloud_Model          = RADAR_EWChaff::MultiCell;
    m_RCS_Model            = RADAR_EWChaff::ResonantDipoleApprox;
    m_RCS_OutputUnit       = RADAR_EWChaff::Linear_m2;
    m_Cell_RCS_Distribution = RADAR_EWChaff::GaussianCenter_Distribution;
    m_NumberOfChaffCell    = 16;
    m_FileName             = "";
    m_Release_Position_XYZData    = { 0.0, 0.0, 0.0 };
    m_Initial_Velocity_XYZData    = { 0.0, 0.0, 0.0 };
    m_Wind_Velocity_XYZData       = { 0.0, 0.0, 0.0 };
    m_Cloud_Initial_Radius_XYZData = { 5.0, 5.0, 5.0 };
    m_Cloud_ExpansionRate_XYZData  = { 2.0, 2.0, 1.0 };
    m_Cloud_MaxRadius_XYZData      = { 100.0, 100.0, 60.0 };
    m_Fall_Speed         = 0.0;
    m_VelocityDecayTime  = 0.5;
    m_Cloud_Lifetime     = 30.0;
    m_CarrierFreq        = 10e9;
    m_DipoleLength       = 0.015;
    m_DipoleLengthSpread = 0.003;
    m_NumberOfDipoles    = 100000.0;
    m_ReferenceDipoleCount = 100000.0;
    m_TotalRCS_Reference = 1000.0;
    m_RCS_GrowthTime     = 0.2;
    m_RCS_DecayTime      = 20.0;
    m_RCS_Floor          = 0.0;
    m_GaussianWeightSigma = 0.65;
    m_TimeStep           = 1e-3;
}

// ============================================================================
// SetAlgoParameters — 设置 m_algo（仅用于端口注册）
// ============================================================================

void RADAR_EWChaff_Block::SetAlgoParameters()
{
    if (!m_algo) return;

    m_algo->Chaff_Mode           = m_Chaff_Mode;
    m_algo->Cloud_Model          = m_Cloud_Model;
    m_algo->RCS_Model            = m_RCS_Model;
    m_algo->RCS_OutputUnit       = m_RCS_OutputUnit;
    m_algo->Cell_RCS_Distribution = m_Cell_RCS_Distribution;
    m_algo->NumberOfChaffCell    = m_NumberOfChaffCell;

    m_algo->FileName = const_cast<char*>(m_FileName.c_str());

    m_algo->Release_Position_XYZ     = m_Release_Position_XYZData.empty()     ? nullptr : m_Release_Position_XYZData.data();
    m_algo->Release_Position_XYZSize = static_cast<int>(m_Release_Position_XYZData.size());
    m_algo->Initial_Velocity_XYZ     = m_Initial_Velocity_XYZData.empty()     ? nullptr : m_Initial_Velocity_XYZData.data();
    m_algo->Initial_Velocity_XYZSize = static_cast<int>(m_Initial_Velocity_XYZData.size());
    m_algo->Wind_Velocity_XYZ        = m_Wind_Velocity_XYZData.empty()        ? nullptr : m_Wind_Velocity_XYZData.data();
    m_algo->Wind_Velocity_XYZSize    = static_cast<int>(m_Wind_Velocity_XYZData.size());
    m_algo->Cloud_Initial_Radius_XYZ = m_Cloud_Initial_Radius_XYZData.empty() ? nullptr : m_Cloud_Initial_Radius_XYZData.data();
    m_algo->Cloud_Initial_Radius_XYZSize = static_cast<int>(m_Cloud_Initial_Radius_XYZData.size());
    m_algo->Cloud_ExpansionRate_XYZ  = m_Cloud_ExpansionRate_XYZData.empty()  ? nullptr : m_Cloud_ExpansionRate_XYZData.data();
    m_algo->Cloud_ExpansionRate_XYZSize = static_cast<int>(m_Cloud_ExpansionRate_XYZData.size());
    m_algo->Cloud_MaxRadius_XYZ      = m_Cloud_MaxRadius_XYZData.empty()      ? nullptr : m_Cloud_MaxRadius_XYZData.data();
    m_algo->Cloud_MaxRadius_XYZSize  = static_cast<int>(m_Cloud_MaxRadius_XYZData.size());

    m_algo->Fall_Speed          = m_Fall_Speed;
    m_algo->VelocityDecayTime   = m_VelocityDecayTime;
    m_algo->Cloud_Lifetime      = m_Cloud_Lifetime;
    m_algo->CarrierFreq         = m_CarrierFreq;
    m_algo->DipoleLength        = m_DipoleLength;
    m_algo->DipoleLengthSpread  = m_DipoleLengthSpread;
    m_algo->NumberOfDipoles     = m_NumberOfDipoles;
    m_algo->ReferenceDipoleCount = m_ReferenceDipoleCount;
    m_algo->TotalRCS_Reference  = m_TotalRCS_Reference;
    m_algo->RCS_GrowthTime      = m_RCS_GrowthTime;
    m_algo->RCS_DecayTime       = m_RCS_DecayTime;
    m_algo->RCS_Floor           = m_RCS_Floor;
    m_algo->GaussianWeightSigma = m_GaussianWeightSigma;
    m_algo->TimeStep            = m_TimeStep;
}

// ============================================================================
// ConvertStringTo — 5 个枚举
// ============================================================================

RADAR_EWChaff::Chaff_ModeEnum RADAR_EWChaff_Block::ConvertStringToChaffMode(const std::string& value)
{
    const std::string v = ToLowerCopy(TrimCopy(value));
    if (v == "fixedcloud"  || v == "0") return RADAR_EWChaff::FixedCloud;
    if (v == "releasedcloud" || v == "1") return RADAR_EWChaff::ReleasedCloud;
    if (v == "userdefined" || v == "user_defined" || v == "2") return RADAR_EWChaff::User_Defined;
    return RADAR_EWChaff::ReleasedCloud;
}

RADAR_EWChaff::Cloud_ModelEnum RADAR_EWChaff_Block::ConvertStringToCloudModel(const std::string& value)
{
    const std::string v = ToLowerCopy(TrimCopy(value));
    if (v == "singlecell" || v == "0") return RADAR_EWChaff::SingleCell;
    if (v == "multicell"  || v == "1") return RADAR_EWChaff::MultiCell;
    return RADAR_EWChaff::MultiCell;
}

RADAR_EWChaff::RCSModelEnum RADAR_EWChaff_Block::ConvertStringToRCSModel(const std::string& value)
{
    const std::string v = ToLowerCopy(TrimCopy(value));
    if (v == "empiricaltotalrcs"  || v == "0") return RADAR_EWChaff::EmpiricalTotalRCS;
    if (v == "resonantdipoleapprox" || v == "1") return RADAR_EWChaff::ResonantDipoleApprox;
    return RADAR_EWChaff::ResonantDipoleApprox;
}

RADAR_EWChaff::RCSOutputUnitEnum RADAR_EWChaff_Block::ConvertStringToRCSOutputUnit(const std::string& value)
{
    const std::string v = ToLowerCopy(TrimCopy(value));
    if (v == "linear_m2" || v == "0") return RADAR_EWChaff::Linear_m2;
    if (v == "dbsm"       || v == "1") return RADAR_EWChaff::dBsm;
    return RADAR_EWChaff::Linear_m2;
}

RADAR_EWChaff::CellRCSDistributionEnum RADAR_EWChaff_Block::ConvertStringToCellRCSDistribution(const std::string& value)
{
    const std::string v = ToLowerCopy(TrimCopy(value));
    if (v == "uniform_distribution"      || v == "0") return RADAR_EWChaff::Uniform_Distribution;
    if (v == "gaussiancenter_distribution" || v == "1") return RADAR_EWChaff::GaussianCenter_Distribution;
    return RADAR_EWChaff::GaussianCenter_Distribution;
}

// ============================================================================
// Vec3 辅助函数
// ============================================================================

RADAR_EWChaff_Block::Vec3 RADAR_EWChaff_Block::m_make_vec(double x, double y, double z)
{
    Vec3 v; v.x = x; v.y = y; v.z = z; return v;
}

RADAR_EWChaff_Block::Vec3 RADAR_EWChaff_Block::m_add(const Vec3& a, const Vec3& b)
{
    return m_make_vec(a.x + b.x, a.y + b.y, a.z + b.z);
}

RADAR_EWChaff_Block::Vec3 RADAR_EWChaff_Block::m_sub(const Vec3& a, const Vec3& b)
{
    return m_make_vec(a.x - b.x, a.y - b.y, a.z - b.z);
}

RADAR_EWChaff_Block::Vec3 RADAR_EWChaff_Block::m_scale(const Vec3& a, double s)
{
    return m_make_vec(a.x * s, a.y * s, a.z * s);
}

RADAR_EWChaff_Block::Vec3 RADAR_EWChaff_Block::m_mul(const Vec3& a, const Vec3& b)
{
    return m_make_vec(a.x * b.x, a.y * b.y, a.z * b.z);
}

double RADAR_EWChaff_Block::m_dot(const Vec3& a, const Vec3& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

double RADAR_EWChaff_Block::m_norm(const Vec3& a)
{
    return std::sqrt(m_dot(a, a));
}

RADAR_EWChaff_Block::Vec3 RADAR_EWChaff_Block::m_normalize(const Vec3& a, const Vec3& fallback)
{
    double n = m_norm(a);
    if (n < 1e-30) return fallback;
    return m_scale(a, 1.0 / n);
}

double RADAR_EWChaff_Block::m_safe_log10(double x)
{
    if (x <= 0.0) return -300.0;
    return std::log10(x);
}

double RADAR_EWChaff_Block::m_get_array_value(const std::vector<double>& data, int idx, double defval)
{
    if (idx >= 0 && idx < static_cast<int>(data.size())) return data[static_cast<std::size_t>(idx)];
    return defval;
}

RADAR_EWChaff_Block::Vec3 RADAR_EWChaff_Block::m_get_array_vec3(const std::vector<double>& data, double defx, double defy, double defz)
{
    return m_make_vec(
        m_get_array_value(data, 0, defx),
        m_get_array_value(data, 1, defy),
        m_get_array_value(data, 2, defz));
}

// ============================================================================
// 算法辅助函数
// ============================================================================

int RADAR_EWChaff_Block::m_active_cell_count() const
{
    if (m_Chaff_Mode == RADAR_EWChaff::User_Defined)
        return (m_NumberOfChaffCell > 0) ? m_NumberOfChaffCell : 1;
    if (m_Cloud_Model == RADAR_EWChaff::SingleCell)
        return 1;
    return (m_NumberOfChaffCell > 0) ? m_NumberOfChaffCell : 1;
}

bool RADAR_EWChaff_Block::m_validate_params() const
{
    if (m_NumberOfChaffCell <= 0) return false;
    if (m_TimeStep <= 0.0) return false;
    if (m_TotalRCS_Reference < 0.0) return false;
    return true;
}

RADAR_EWChaff_Block::Vec3 RADAR_EWChaff_Block::m_get_release_position_param() const
{
    return m_get_array_vec3(m_Release_Position_XYZData, 0.0, 0.0, 0.0);
}

RADAR_EWChaff_Block::Vec3 RADAR_EWChaff_Block::m_get_initial_velocity_param() const
{
    return m_get_array_vec3(m_Initial_Velocity_XYZData, 0.0, 0.0, 0.0);
}

RADAR_EWChaff_Block::Vec3 RADAR_EWChaff_Block::m_get_wind_velocity() const
{
    return m_get_array_vec3(m_Wind_Velocity_XYZData, 0.0, 0.0, 0.0);
}

RADAR_EWChaff_Block::Vec3 RADAR_EWChaff_Block::m_get_initial_radius() const
{
    return m_get_array_vec3(m_Cloud_Initial_Radius_XYZData, 5.0, 5.0, 5.0);
}

RADAR_EWChaff_Block::Vec3 RADAR_EWChaff_Block::m_get_expansion_rate() const
{
    return m_get_array_vec3(m_Cloud_ExpansionRate_XYZData, 0.0, 0.0, 0.0);
}

RADAR_EWChaff_Block::Vec3 RADAR_EWChaff_Block::m_get_max_radius() const
{
    return m_get_array_vec3(m_Cloud_MaxRadius_XYZData, -1.0, -1.0, -1.0);
}

double RADAR_EWChaff_Block::m_get_carrier_freq()
{
    // 从输入端口 7 (CarrierFreqIn) 读取，由 Run 中 push 提供
    // 这里用参数默认值
    return m_CarrierFreq;
}

RADAR_EWChaff_Block::Vec3 RADAR_EWChaff_Block::m_get_platform_position()
{
    Vec3 p = m_get_release_position_param();
    // 由 Run 中的 m_platformX/Y/Z 提供
    return p;
}

RADAR_EWChaff_Block::Vec3 RADAR_EWChaff_Block::m_get_platform_velocity()
{
    Vec3 v = m_get_initial_velocity_param();
    return v;
}

void RADAR_EWChaff_Block::m_compute_cloud(double age, Vec3& center, Vec3& centerVel, Vec3& radius, Vec3& radiusRate) const
{
    const Vec3 initRadius = m_get_initial_radius();
    const Vec3 expansion  = m_get_expansion_rate();
    const Vec3 maxRadius  = m_get_max_radius();

    if (m_Chaff_Mode == RADAR_EWChaff::FixedCloud) {
        center    = m_get_release_position_param();
        centerVel = m_make_vec(0.0, 0.0, 0.0);
        radius    = initRadius;
        radiusRate = m_make_vec(0.0, 0.0, 0.0);
        return;
    }

    const Vec3 windVel = m_get_wind_velocity();
    const double tau = (m_VelocityDecayTime > 0.0) ? m_VelocityDecayTime : 0.0;

    Vec3 memoryDisp = m_make_vec(0.0, 0.0, 0.0);
    Vec3 memoryVel  = m_make_vec(0.0, 0.0, 0.0);

    if (tau > 0.0) {
        const double e = std::exp(-age / tau);
        memoryDisp = m_scale(m_releaseVelLatched, tau * (1.0 - e));
        memoryVel  = m_scale(m_releaseVelLatched, e);
    }

    const Vec3 windDisp  = m_scale(windVel, age);
    const Vec3 fallDisp  = m_make_vec(0.0, 0.0, -m_Fall_Speed * age);

    center    = m_add(m_add(m_add(m_releasePosLatched, windDisp), memoryDisp), fallDisp);
    centerVel = m_add(m_add(windVel, memoryVel), m_make_vec(0.0, 0.0, -m_Fall_Speed));

    Vec3 rawRadius = m_add(initRadius, m_scale(expansion, age));

    radius.x = (maxRadius.x > 0.0) ? std::min(rawRadius.x, maxRadius.x) : rawRadius.x;
    radius.y = (maxRadius.y > 0.0) ? std::min(rawRadius.y, maxRadius.y) : rawRadius.y;
    radius.z = (maxRadius.z > 0.0) ? std::min(rawRadius.z, maxRadius.z) : rawRadius.z;

    radiusRate.x = ((maxRadius.x > 0.0) && (rawRadius.x >= maxRadius.x)) ? 0.0 : expansion.x;
    radiusRate.y = ((maxRadius.y > 0.0) && (rawRadius.y >= maxRadius.y)) ? 0.0 : expansion.y;
    radiusRate.z = ((maxRadius.z > 0.0) && (rawRadius.z >= maxRadius.z)) ? 0.0 : expansion.z;
}

RADAR_EWChaff_Block::Vec3 RADAR_EWChaff_Block::m_cell_pattern(int idx, int n) const
{
    if (n <= 1) return m_make_vec(0.0, 0.0, 0.0);

    const double i = static_cast<double>(idx);
    const double N = static_cast<double>(n);
    const double z  = 1.0 - 2.0 * (i + 0.5) / N;
    const double rxy = std::sqrt(std::max(0.0, 1.0 - z * z));
    const double phi = kGoldenAngle * i;
    const double frac = std::fmod((i + 1.0) * 0.6180339887498948482, 1.0);
    const double radial = std::pow(std::max(0.05, frac), 1.0 / 3.0);

    return m_make_vec(
        radial * rxy * std::cos(phi),
        radial * rxy * std::sin(phi),
        radial * z);
}

void RADAR_EWChaff_Block::m_compute_cell_weights(const std::vector<Vec3>& patterns, std::vector<double>& weights) const
{
    const int n = static_cast<int>(patterns.size());
    weights.assign(static_cast<std::size_t>(n), 0.0);
    if (n <= 0) return;

    if (n == 1 || m_Cell_RCS_Distribution == RADAR_EWChaff::Uniform_Distribution) {
        const double w = 1.0 / static_cast<double>(n);
        for (int i = 0; i < n; ++i) weights[static_cast<std::size_t>(i)] = w;
        return;
    }

    const double sigma = (m_GaussianWeightSigma > 0.0) ? m_GaussianWeightSigma : 0.65;
    double sumW = 0.0;
    for (int i = 0; i < n; ++i) {
        const double rho2 = m_dot(patterns[static_cast<std::size_t>(i)], patterns[static_cast<std::size_t>(i)]);
        const double w = std::exp(-0.5 * rho2 / (sigma * sigma));
        weights[static_cast<std::size_t>(i)] = w;
        sumW += w;
    }
    if (sumW <= 0.0) {
        const double w = 1.0 / static_cast<double>(n);
        for (int i = 0; i < n; ++i) weights[static_cast<std::size_t>(i)] = w;
        return;
    }
    for (int i = 0; i < n; ++i) weights[static_cast<std::size_t>(i)] /= sumW;
}

double RADAR_EWChaff_Block::m_frequency_match_factor(double freqHz) const
{
    if (freqHz <= 0.0 || m_DipoleLength <= 0.0) return 0.0;
    const double lambda = kSpeedOfLight / freqHz;
    const double halfLambda = 0.5 * lambda;
    double spread = m_DipoleLengthSpread;
    if (spread <= 0.0) spread = std::max(1.0e-9, 0.05 * halfLambda);
    const double diff = m_DipoleLength - halfLambda;
    return std::exp(-(diff * diff) / (2.0 * spread * spread));
}

double RADAR_EWChaff_Block::m_total_rcs_linear(double age, double freqHz, bool active) const
{
    if (!active) return 0.0;

    double growth = 1.0;
    double decay  = 1.0;
    if (m_Chaff_Mode == RADAR_EWChaff::ReleasedCloud) {
        if (m_RCS_GrowthTime > 0.0) growth = 1.0 - std::exp(-age / m_RCS_GrowthTime);
        if (m_RCS_DecayTime  > 0.0) decay  = std::exp(-age / m_RCS_DecayTime);
    }

    double total = m_TotalRCS_Reference * growth * decay;

    if (m_RCS_Model == RADAR_EWChaff::ResonantDipoleApprox) {
        const double match = m_frequency_match_factor(freqHz);
        const double refCount = (m_ReferenceDipoleCount > 0.0) ? m_ReferenceDipoleCount : 1.0;
        const double countScale = std::max(0.0, m_NumberOfDipoles) / refCount;
        total *= match * countScale;
    }

    if (total > 0.0 && m_RCS_Floor > 0.0) total = std::max(total, m_RCS_Floor);
    if (total < 0.0) total = 0.0;
    return total;
}

// ============================================================================
// Setup
// ============================================================================

bool RADAR_EWChaff_Block::Setup()
{
    Block::Setup();

    SetAlgoParameters();

    // 清空变步长缓冲队列
    m_inputBuffer.clear();
    while (!m_outputQueue.empty()) m_outputQueue.pop();

    // 初始化 Block 自身的算法状态（不调用 m_algo->Setup()，避免双重初始化）
    if (m_NumberOfChaffCell <= 0) m_NumberOfChaffCell = 1;
    m_sampleIndex = 0;
    m_userPathIndex = 0;
    m_released = false;
    m_releaseSampleIndex = 0;
    m_releasePosLatched = m_get_release_position_param();
    m_releaseVelLatched = m_get_initial_velocity_param();

    // user file loading (User_Defined mode)
    m_userPath.clear();
    m_lastUserSample = UserSample();
    if (m_Chaff_Mode == RADAR_EWChaff::User_Defined && !m_FileName.empty()) {
        std::ifstream ifs(m_FileName);
        if (ifs.is_open()) {
            std::string line;
            while (std::getline(ifs, line)) {
                std::istringstream iss(line);
                const int n = m_NumberOfChaffCell;
                UserSample sample;
                sample.pos.reserve(static_cast<std::size_t>(n));
                sample.rcs.reserve(static_cast<std::size_t>(n));
                sample.vel.reserve(static_cast<std::size_t>(n));
                sample.valid.reserve(static_cast<std::size_t>(n));
                bool ok = true;
                for (int i = 0; i < n; ++i) {
                    double x = 0, y = 0, z = 0, r = 0, vx = 0, vy = 0, vz = 0, valid = 0;
                    if (!(iss >> x >> y >> z >> r >> vx >> vy >> vz >> valid)) { ok = false; break; }
                    sample.pos.push_back(m_make_vec(x, y, z));
                    sample.rcs.push_back(r);
                    sample.vel.push_back(m_make_vec(vx, vy, vz));
                    sample.valid.push_back(valid);
                }
                if (ok && static_cast<int>(sample.pos.size()) == n)
                    m_userPath.push_back(sample);
            }
        }
        if (!m_userPath.empty()) m_lastUserSample = m_userPath.front();
    }

    return m_validate_params();
}

// ============================================================================
// Run — 双模式分发
// ============================================================================

bool RADAR_EWChaff_Block::Run()
{
    if (IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

// ============================================================================
// DataStreamRun — 完全内联算法逻辑，不调用 m_algo->Run()
// ============================================================================

bool RADAR_EWChaff_Block::DataStreamRun()
{
    // ① 读取控制输入
    double releaseVal = 0;
    double platX = 0, platY = 0, platZ = 0;
    double platVx = 0, platVy = 0, platVz = 0;
    double carrierFreqIn = m_CarrierFreq;

    {
        auto d = ReadInputData<double>(GetInputPortName(0));
        if (!d.empty()) releaseVal = d[0];
    }
    {
        auto d = ReadInputData<double>(GetInputPortName(1));
        if (!d.empty()) platX = d[0];
    }
    {
        auto d = ReadInputData<double>(GetInputPortName(2));
        if (!d.empty()) platY = d[0];
    }
    {
        auto d = ReadInputData<double>(GetInputPortName(3));
        if (!d.empty()) platZ = d[0];
    }
    {
        auto d = ReadInputData<double>(GetInputPortName(4));
        if (!d.empty()) platVx = d[0];
    }
    {
        auto d = ReadInputData<double>(GetInputPortName(5));
        if (!d.empty()) platVy = d[0];
    }
    {
        auto d = ReadInputData<double>(GetInputPortName(6));
        if (!d.empty()) platVz = d[0];
    }
    {
        auto d = ReadInputData<double>(GetInputPortName(7));
        if (!d.empty()) carrierFreqIn = d[0];
    }

    // ===== 算法核心逻辑（RADAR_EWChaff::Run 内联） =====
    const int n = m_active_cell_count();

    // User_Defined 模式
    if (m_Chaff_Mode == RADAR_EWChaff::User_Defined) {
        UserSample out = m_lastUserSample;
        if (!m_userPath.empty()) {
            if (m_userPathIndex < m_userPath.size()) {
                out = m_userPath[m_userPathIndex];
                m_lastUserSample = out;
                ++m_userPathIndex;
            } else {
                out = m_lastUserSample;
            }
        }
        // 写输出
        std::vector<SystemVueModelBuilder::DoubleMatrix> posOut(static_cast<std::size_t>(n));
        std::vector<double> rcsOut(static_cast<std::size_t>(n));
        std::vector<SystemVueModelBuilder::DoubleMatrix> velOut(static_cast<std::size_t>(n));
        std::vector<double> validOut(static_cast<std::size_t>(n));
        for (int i = 0; i < n; ++i) {
            SystemVueModelBuilder::DoubleMatrix pm(3, 1);
            pm(0, 0) = (i < static_cast<int>(out.pos.size())) ? out.pos[static_cast<std::size_t>(i)].x : 0.0;
            pm(1, 0) = (i < static_cast<int>(out.pos.size())) ? out.pos[static_cast<std::size_t>(i)].y : 0.0;
            pm(2, 0) = (i < static_cast<int>(out.pos.size())) ? out.pos[static_cast<std::size_t>(i)].z : 0.0;
            posOut[static_cast<std::size_t>(i)] = pm;
            rcsOut[static_cast<std::size_t>(i)] = (i < static_cast<int>(out.rcs.size())) ? out.rcs[static_cast<std::size_t>(i)] : 0.0;
            SystemVueModelBuilder::DoubleMatrix vm(3, 1);
            vm(0, 0) = (i < static_cast<int>(out.vel.size())) ? out.vel[static_cast<std::size_t>(i)].x : 0.0;
            vm(1, 0) = (i < static_cast<int>(out.vel.size())) ? out.vel[static_cast<std::size_t>(i)].y : 0.0;
            vm(2, 0) = (i < static_cast<int>(out.vel.size())) ? out.vel[static_cast<std::size_t>(i)].z : 0.0;
            velOut[static_cast<std::size_t>(i)] = vm;
            validOut[static_cast<std::size_t>(i)] = (i < static_cast<int>(out.valid.size())) ? out.valid[static_cast<std::size_t>(i)] : 0.0;
        }
        WriteOutputData(GetOutputPortName(0), posOut);
        WriteOutputData(GetOutputPortName(1), rcsOut);
        WriteOutputData(GetOutputPortName(2), velOut);
        WriteOutputData(GetOutputPortName(3), validOut);
        ++m_sampleIndex;
        return true;
    }

    // ReleasedCloud / FixedCloud 模式
    if (m_Chaff_Mode == RADAR_EWChaff::ReleasedCloud) {
        const bool releaseSignal = (releaseVal > 0.0);
        if (!m_released && releaseSignal) {
            m_released = true;
            m_releaseSampleIndex = m_sampleIndex;
            m_releasePosLatched = m_make_vec(platX, platY, platZ);
            m_releaseVelLatched = m_make_vec(platVx, platVy, platVz);
        }
    } else {
        // FixedCloud
        m_released = true;
        m_releaseSampleIndex = 0;
        m_releasePosLatched = m_get_release_position_param();
        m_releaseVelLatched = m_make_vec(0.0, 0.0, 0.0);
    }

    bool active = m_released;
    double age = 0.0;
    if (active) {
        if (m_Chaff_Mode == RADAR_EWChaff::ReleasedCloud)
            age = static_cast<double>(m_sampleIndex - m_releaseSampleIndex) * m_TimeStep;
        else
            age = 0.0;
    }

    if (active && m_Chaff_Mode == RADAR_EWChaff::ReleasedCloud && m_Cloud_Lifetime > 0.0 && age > m_Cloud_Lifetime)
        active = false;

    if (!active) {
        std::vector<SystemVueModelBuilder::DoubleMatrix> posOut(static_cast<std::size_t>(n));
        std::vector<double> rcsOut(static_cast<std::size_t>(n), 0.0);
        std::vector<SystemVueModelBuilder::DoubleMatrix> velOut(static_cast<std::size_t>(n));
        std::vector<double> validOut(static_cast<std::size_t>(n), 0.0);
        for (int i = 0; i < n; ++i) {
            SystemVueModelBuilder::DoubleMatrix zm(3, 1); zm(0,0)=0; zm(1,0)=0; zm(2,0)=0;
            posOut[static_cast<std::size_t>(i)] = zm;
            velOut[static_cast<std::size_t>(i)] = zm;
        }
        WriteOutputData(GetOutputPortName(0), posOut);
        WriteOutputData(GetOutputPortName(1), rcsOut);
        WriteOutputData(GetOutputPortName(2), velOut);
        WriteOutputData(GetOutputPortName(3), validOut);
        ++m_sampleIndex;
        return true;
    }

    // 活跃状态：计算云团
    Vec3 center, centerVel, radius, radiusRate;
    m_compute_cloud(age, center, centerVel, radius, radiusRate);

    std::vector<Vec3> patterns;
    patterns.reserve(static_cast<std::size_t>(n));
    for (int i = 0; i < n; ++i)
        patterns.push_back(m_cell_pattern(i, n));

    std::vector<double> weights;
    m_compute_cell_weights(patterns, weights);

    const double freqHz = (carrierFreqIn > 0.0) ? carrierFreqIn : m_CarrierFreq;
    const double totalRCS = m_total_rcs_linear(age, freqHz, true);

    std::vector<Vec3> positions;
    std::vector<double> rcsValues;
    std::vector<Vec3> velocities;
    std::vector<double> validValues;
    positions.reserve(static_cast<std::size_t>(n));
    rcsValues.reserve(static_cast<std::size_t>(n));
    velocities.reserve(static_cast<std::size_t>(n));
    validValues.reserve(static_cast<std::size_t>(n));

    for (int i = 0; i < n; ++i) {
        const Vec3 pat = patterns[static_cast<std::size_t>(i)];
        const Vec3 offset = m_mul(pat, radius);
        const Vec3 pos = m_add(center, offset);
        const Vec3 expansionVel = m_mul(pat, radiusRate);
        const Vec3 vel = m_add(centerVel, expansionVel);

        double rcs = totalRCS * weights[static_cast<std::size_t>(i)];
        if (m_RCS_OutputUnit == RADAR_EWChaff::dBsm)
            rcs = 10.0 * m_safe_log10(rcs);

        positions.push_back(pos);
        rcsValues.push_back(rcs);
        velocities.push_back(vel);
        validValues.push_back(1.0);
    }

    // 写输出
    std::vector<SystemVueModelBuilder::DoubleMatrix> posOut(static_cast<std::size_t>(n));
    std::vector<double> rcsOut(static_cast<std::size_t>(n));
    std::vector<SystemVueModelBuilder::DoubleMatrix> velOut(static_cast<std::size_t>(n));
    std::vector<double> validOut(static_cast<std::size_t>(n));
    for (int i = 0; i < n; ++i) {
        SystemVueModelBuilder::DoubleMatrix pm(3, 1);
        pm(0, 0) = positions[static_cast<std::size_t>(i)].x;
        pm(1, 0) = positions[static_cast<std::size_t>(i)].y;
        pm(2, 0) = positions[static_cast<std::size_t>(i)].z;
        posOut[static_cast<std::size_t>(i)] = pm;
        rcsOut[static_cast<std::size_t>(i)] = rcsValues[static_cast<std::size_t>(i)];
        SystemVueModelBuilder::DoubleMatrix vm(3, 1);
        vm(0, 0) = velocities[static_cast<std::size_t>(i)].x;
        vm(1, 0) = velocities[static_cast<std::size_t>(i)].y;
        vm(2, 0) = velocities[static_cast<std::size_t>(i)].z;
        velOut[static_cast<std::size_t>(i)] = vm;
        validOut[static_cast<std::size_t>(i)] = validValues[static_cast<std::size_t>(i)];
    }

    WriteOutputData(GetOutputPortName(0), posOut);
    WriteOutputData(GetOutputPortName(1), rcsOut);
    WriteOutputData(GetOutputPortName(2), velOut);
    WriteOutputData(GetOutputPortName(3), validOut);

    ++m_sampleIndex;
    return true;
}

// ============================================================================
// TimeDrivenRun — 变步长模式：输入存 buffer，处理后入队，队列非空则输出
// ============================================================================

bool RADAR_EWChaff_Block::TimeDrivenRun()
{
    // ① 累积输入到 m_inputBuffer
    {
        auto d0 = ReadInputData<double>(GetInputPortName(0));
        auto d1 = ReadInputData<double>(GetInputPortName(1));
        auto d2 = ReadInputData<double>(GetInputPortName(2));
        auto d3 = ReadInputData<double>(GetInputPortName(3));
        auto d4 = ReadInputData<double>(GetInputPortName(4));
        auto d5 = ReadInputData<double>(GetInputPortName(5));
        auto d6 = ReadInputData<double>(GetInputPortName(6));
        auto d7 = ReadInputData<double>(GetInputPortName(7));

        if (!d0.empty() || !d1.empty() || !d2.empty() || !d3.empty() ||
            !d4.empty() || !d5.empty() || !d6.empty() || !d7.empty()) {
            InputSnapshot in;
            in.release       = d0.empty() ? 0.0 : d0[0];
            in.platX         = d1.empty() ? 0.0 : d1[0];
            in.platY         = d2.empty() ? 0.0 : d2[0];
            in.platZ         = d3.empty() ? 0.0 : d3[0];
            in.platVx        = d4.empty() ? 0.0 : d4[0];
            in.platVy        = d5.empty() ? 0.0 : d5[0];
            in.platVz        = d6.empty() ? 0.0 : d6[0];
            in.carrierFreqIn = d7.empty() ? m_CarrierFreq : d7[0];
            m_inputBuffer.push_back(in);
        }
    }

    // ② 处理所有累积输入 → 入队 m_outputQueue
    while (!m_inputBuffer.empty()) {
        InputSnapshot in = m_inputBuffer.front();
        m_inputBuffer.erase(m_inputBuffer.begin());

        const int n = m_active_cell_count();

        ChaffOutput out;
        out.pos.resize(static_cast<std::size_t>(n));
        out.rcs.resize(static_cast<std::size_t>(n));
        out.vel.resize(static_cast<std::size_t>(n));
        out.valid.resize(static_cast<std::size_t>(n));

        // --- User_Defined 模式 ---
        if (m_Chaff_Mode == RADAR_EWChaff::User_Defined) {
            UserSample us = m_lastUserSample;
            if (!m_userPath.empty()) {
                if (m_userPathIndex < m_userPath.size()) {
                    us = m_userPath[m_userPathIndex];
                    m_lastUserSample = us;
                    ++m_userPathIndex;
                } else {
                    us = m_lastUserSample;
                }
            }
            for (int i = 0; i < n; ++i) {
                SystemVueModelBuilder::DoubleMatrix pm(3, 1);
                pm(0, 0) = (i < static_cast<int>(us.pos.size())) ? us.pos[static_cast<std::size_t>(i)].x : 0.0;
                pm(1, 0) = (i < static_cast<int>(us.pos.size())) ? us.pos[static_cast<std::size_t>(i)].y : 0.0;
                pm(2, 0) = (i < static_cast<int>(us.pos.size())) ? us.pos[static_cast<std::size_t>(i)].z : 0.0;
                out.pos[i] = pm;
                out.rcs[i] = (i < static_cast<int>(us.rcs.size())) ? us.rcs[static_cast<std::size_t>(i)] : 0.0;
                SystemVueModelBuilder::DoubleMatrix vm(3, 1);
                vm(0, 0) = (i < static_cast<int>(us.vel.size())) ? us.vel[static_cast<std::size_t>(i)].x : 0.0;
                vm(1, 0) = (i < static_cast<int>(us.vel.size())) ? us.vel[static_cast<std::size_t>(i)].y : 0.0;
                vm(2, 0) = (i < static_cast<int>(us.vel.size())) ? us.vel[static_cast<std::size_t>(i)].z : 0.0;
                out.vel[i] = vm;
                out.valid[i] = (i < static_cast<int>(us.valid.size())) ? us.valid[static_cast<std::size_t>(i)] : 0.0;
            }
            ++m_sampleIndex;
            m_outputQueue.push(out);
            continue;
        }

        // --- ReleasedCloud / FixedCloud 模式 ---
        if (m_Chaff_Mode == RADAR_EWChaff::ReleasedCloud) {
            const bool releaseSignal = (in.release > 0.0);
            if (!m_released && releaseSignal) {
                m_released = true;
                m_releaseSampleIndex = m_sampleIndex;
                m_releasePosLatched = m_make_vec(in.platX, in.platY, in.platZ);
                m_releaseVelLatched = m_make_vec(in.platVx, in.platVy, in.platVz);
            }
        } else {
            m_released = true;
            m_releaseSampleIndex = 0;
            m_releasePosLatched = m_get_release_position_param();
            m_releaseVelLatched = m_make_vec(0.0, 0.0, 0.0);
        }

        bool active = m_released;
        double age = 0.0;
        if (active) {
            if (m_Chaff_Mode == RADAR_EWChaff::ReleasedCloud)
                age = static_cast<double>(m_sampleIndex - m_releaseSampleIndex) * m_TimeStep;
            else
                age = 0.0;
        }

        if (active && m_Chaff_Mode == RADAR_EWChaff::ReleasedCloud && m_Cloud_Lifetime > 0.0 && age > m_Cloud_Lifetime)
            active = false;

        if (!active) {
            for (int i = 0; i < n; ++i) {
                SystemVueModelBuilder::DoubleMatrix zm(3, 1); zm(0,0)=0; zm(1,0)=0; zm(2,0)=0;
                out.pos[i] = zm;
                out.rcs[i] = 0.0;
                out.vel[i] = zm;
                out.valid[i] = 0.0;
            }
            ++m_sampleIndex;
            m_outputQueue.push(out);
            continue;
        }

        // 活跃状态：计算云团
        Vec3 center, centerVel, radius, radiusRate;
        m_compute_cloud(age, center, centerVel, radius, radiusRate);

        std::vector<Vec3> patterns;
        patterns.reserve(static_cast<std::size_t>(n));
        for (int i = 0; i < n; ++i)
            patterns.push_back(m_cell_pattern(i, n));

        std::vector<double> weights;
        m_compute_cell_weights(patterns, weights);

        const double freqHz = (in.carrierFreqIn > 0.0) ? in.carrierFreqIn : m_CarrierFreq;
        const double totalRCS = m_total_rcs_linear(age, freqHz, true);

        for (int i = 0; i < n; ++i) {
            const Vec3 pat = patterns[static_cast<std::size_t>(i)];
            const Vec3 offset = m_mul(pat, radius);
            const Vec3 pos = m_add(center, offset);
            const Vec3 expansionVel = m_mul(pat, radiusRate);
            const Vec3 vel = m_add(centerVel, expansionVel);

            double rcs = totalRCS * weights[static_cast<std::size_t>(i)];
            if (m_RCS_OutputUnit == RADAR_EWChaff::dBsm)
                rcs = 10.0 * m_safe_log10(rcs);

            SystemVueModelBuilder::DoubleMatrix pm(3, 1);
            pm(0, 0) = pos.x; pm(1, 0) = pos.y; pm(2, 0) = pos.z;
            out.pos[i] = pm;
            out.rcs[i] = rcs;
            SystemVueModelBuilder::DoubleMatrix vm(3, 1);
            vm(0, 0) = vel.x; vm(1, 0) = vel.y; vm(2, 0) = vel.z;
            out.vel[i] = vm;
            out.valid[i] = 1.0;
        }

        ++m_sampleIndex;
        m_outputQueue.push(out);
    }

    // ③ 出队写入：outputQueue 不为空就输出一次
    if (!m_outputQueue.empty()) {
        ChaffOutput out = m_outputQueue.front();
        m_outputQueue.pop();
        WriteOutputData(GetOutputPortName(0), out.pos);
        WriteOutputData(GetOutputPortName(1), out.rcs);
        WriteOutputData(GetOutputPortName(2), out.vel);
        WriteOutputData(GetOutputPortName(3), out.valid);
    }

    return true;
}

// ============================================================================
// Initialize
// ============================================================================

bool RADAR_EWChaff_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_algo = std::make_unique<RADAR_EWChaff>();

    SetDefaultParameters();

    try { m_Chaff_Mode           = ConvertStringToChaffMode(getParameter("Chaff_Mode").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Chaff_Mode', using default value."); }
    try { m_Cloud_Model          = ConvertStringToCloudModel(getParameter("Cloud_Model").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Cloud_Model', using default value."); }
    try { m_RCS_Model            = ConvertStringToRCSModel(getParameter("RCS_Model").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'RCS_Model', using default value."); }
    try { m_RCS_OutputUnit       = ConvertStringToRCSOutputUnit(getParameter("RCS_OutputUnit").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'RCS_OutputUnit', using default value."); }
    try { m_Cell_RCS_Distribution = ConvertStringToCellRCSDistribution(getParameter("Cell_RCS_Distribution").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Cell_RCS_Distribution', using default value."); }
    try { m_NumberOfChaffCell    = std::stoi(getParameter("NumberOfChaffCell").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'NumberOfChaffCell', using default value."); }
    try { m_FileName = getParameter("FileName").Value; } catch (...) { LOG_WARN("Failed to parse parameter 'FileName', using default value."); }

    try { m_Release_Position_XYZData    = ParseStringToDoubleVector(getParameter("Release_Position_XYZ").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Release_Position_XYZ', using default value."); }
    try { m_Initial_Velocity_XYZData    = ParseStringToDoubleVector(getParameter("Initial_Velocity_XYZ").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Initial_Velocity_XYZ', using default value."); }
    try { m_Wind_Velocity_XYZData       = ParseStringToDoubleVector(getParameter("Wind_Velocity_XYZ").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Wind_Velocity_XYZ', using default value."); }
    try { m_Cloud_Initial_Radius_XYZData = ParseStringToDoubleVector(getParameter("Cloud_Initial_Radius_XYZ").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Cloud_Initial_Radius_XYZ', using default value."); }
    try { m_Cloud_ExpansionRate_XYZData  = ParseStringToDoubleVector(getParameter("Cloud_ExpansionRate_XYZ").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Cloud_ExpansionRate_XYZ', using default value."); }
    try { m_Cloud_MaxRadius_XYZData      = ParseStringToDoubleVector(getParameter("Cloud_MaxRadius_XYZ").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Cloud_MaxRadius_XYZ', using default value."); }

    try { m_Fall_Speed         = std::stod(getParameter("Fall_Speed").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Fall_Speed', using default value."); }
    try { m_VelocityDecayTime  = std::stod(getParameter("VelocityDecayTime").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'VelocityDecayTime', using default value."); }
    try { m_Cloud_Lifetime     = std::stod(getParameter("Cloud_Lifetime").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Cloud_Lifetime', using default value."); }
    try { m_CarrierFreq        = std::stod(getParameter("CarrierFreq").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'CarrierFreq', using default value."); }
    try { m_DipoleLength       = std::stod(getParameter("DipoleLength").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'DipoleLength', using default value."); }
    try { m_DipoleLengthSpread = std::stod(getParameter("DipoleLengthSpread").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'DipoleLengthSpread', using default value."); }
    try { m_NumberOfDipoles    = std::stod(getParameter("NumberOfDipoles").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'NumberOfDipoles', using default value."); }
    try { m_ReferenceDipoleCount = std::stod(getParameter("ReferenceDipoleCount").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'ReferenceDipoleCount', using default value."); }
    try { m_TotalRCS_Reference = std::stod(getParameter("TotalRCS_Reference").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'TotalRCS_Reference', using default value."); }
    try { m_RCS_GrowthTime     = std::stod(getParameter("RCS_GrowthTime").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'RCS_GrowthTime', using default value."); }
    try { m_RCS_DecayTime      = std::stod(getParameter("RCS_DecayTime").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'RCS_DecayTime', using default value."); }
    try { m_RCS_Floor          = std::stod(getParameter("RCS_Floor").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'RCS_Floor', using default value."); }
    try { m_GaussianWeightSigma = std::stod(getParameter("GaussianWeightSigma").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'GaussianWeightSigma', using default value."); }
    try { m_TimeStep           = std::stod(getParameter("TimeStep").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'TimeStep', using default value."); }

    SetAlgoParameters();

    if (!m_algo->Setup()) return false;

    // 输入端口
    AddInputPort("Release",       m_algo->Release,       1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddInputPort("PlatformX",     m_algo->PlatformX,     1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddInputPort("PlatformY",     m_algo->PlatformY,     1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddInputPort("PlatformZ",     m_algo->PlatformZ,     1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddInputPort("PlatformVx",    m_algo->PlatformVx,    1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddInputPort("PlatformVy",    m_algo->PlatformVy,    1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddInputPort("PlatformVz",    m_algo->PlatformVz,    1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddInputPort("CarrierFreqIn", m_algo->CarrierFreqIn, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);

    // 输出端口
    AddOutputPort("ChaffPos",  m_algo->ChaffPos,  m_NumberOfChaffCell, Block::DataType::MATRIX_DOUBLE_BUS);
    AddOutputPort("ChaffRCS",  m_algo->ChaffRCS,  m_NumberOfChaffCell, Block::DataType::DOUBLE_BUS);
    AddOutputPort("ChaffVel",  m_algo->ChaffVel,  m_NumberOfChaffCell, Block::DataType::MATRIX_DOUBLE_BUS);
    AddOutputPort("ValidFlag", m_algo->ValidFlag, m_NumberOfChaffCell, Block::DataType::DOUBLE_BUS);

    return true;
}
