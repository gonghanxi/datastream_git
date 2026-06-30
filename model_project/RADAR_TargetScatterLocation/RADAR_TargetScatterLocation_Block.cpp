#include "RADAR_TargetScatterLocation_Block.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

// ============================================================================
// 常数
// ============================================================================

static const double kPi              = 3.1415926535897932384626433832795;
static const double kDegToRad        = kPi / 180.0;
static const double kEarthSemiMajor  = 6378137.0;
static const double kEarthSemiMinor  = 6356752.0;
static const double kEarthRotRate    = 7.2921151467e-5;

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
    std::string s = TrimCopy(value);
    if (s.empty()) return result;
    if (s.front() == '[') s = s.substr(1);
    if (!s.empty() && s.back() == ']') s = s.substr(0, s.length() - 1);
    std::stringstream ss(s);
    std::string token;
    while (std::getline(ss, token, ',')) {
        token = TrimCopy(token);
        if (!token.empty()) {
            try { result.push_back(std::stod(token)); }
            catch (...) { result.push_back(0.0); }
        }
    }
    return result;
}

} // anonymous namespace

// ============================================================================
// 构造函数
// ============================================================================

RADAR_TargetScatterLocation_Block::RADAR_TargetScatterLocation_Block(const std::string& name)
    : Block(name)
    , m_Trajectory_Mode(RADAR_TargetScatterLocation::ECI_Frame)
    , m_IsRandomError(RADAR_TargetScatterLocation::RandomError_false)
    , m_IsRCSRandom(RADAR_TargetScatterLocation::RCSRandom_false)
    , m_NumberOfTargetScatter(1)
    , m_Velocity_Initial(0.0)
    , m_Accelerate_Initial(0.0)
    , m_Accelerate_Variance(0.0)
    , m_TimeStep(1e-9)
    , m_sampleIndex(0)
    , m_p0Ecef(m_make_vec(0.0, 0.0, 0.0))
    , m_eastEcef(m_make_vec(0.0, 1.0, 0.0))
    , m_northEcef(m_make_vec(0.0, 0.0, 1.0))
    , m_upEcef(m_make_vec(1.0, 0.0, 0.0))
    , m_motionAccumEcef(m_make_vec(0.0, 0.0, 0.0))
    , m_lonRad(0.0)
    , m_latRad(0.0)
    , m_userPathIndex(0)
{
    initEmptyUserSample();
}

// ============================================================================
// SetDefaultParameters
// ============================================================================

void RADAR_TargetScatterLocation_Block::SetDefaultParameters()
{
    m_Trajectory_Mode       = RADAR_TargetScatterLocation::ECI_Frame;
    m_IsRandomError         = RADAR_TargetScatterLocation::RandomError_false;
    m_IsRCSRandom           = RADAR_TargetScatterLocation::RCSRandom_false;
    m_NumberOfTargetScatter = 1;
    m_FileName              = "";
    m_ScatterLocData        = { 0.0, 0.0, 0.0 };
    m_RCSData               = { 1.0 };
    m_RCS_VarianceData      = { 0.0 };
    m_DurationTimeData      = { 1.0 };
    m_Position_InitialData  = { 0.0, 0.0, 0.0 };
    m_Position_Initial_XYZData  = { 0.0, 20e3, 5e3 };
    m_Velocity_Initial_XYZData  = { 0.0, 0.0, 0.0 };
    m_Accelerate_XYZData      = { 0.0, 0.0, 0.0 };
    m_Jerk_XYZData            = { 0.0, 0.0, 0.0 };
    m_Velocity_Initial      = 0.0;
    m_Accelerate_Initial    = 0.0;
    m_Accelerate_Variance   = 0.0;
    m_TimeStep              = 1e-9;
}

// ============================================================================
// SetAlgoParameters
// ============================================================================

void RADAR_TargetScatterLocation_Block::SetAlgoParameters()
{
    if (!m_algo) return;

    m_algo->Trajectory_Mode       = m_Trajectory_Mode;
    m_algo->NumberOfTargetScatter = m_NumberOfTargetScatter;
    m_algo->IsRandomError         = m_IsRandomError;
    m_algo->IsRCSRandom           = m_IsRCSRandom;
    m_algo->FileName              = const_cast<char*>(m_FileName.c_str());

    m_algo->ScatterLoc      = m_ScatterLocData.empty()      ? nullptr : m_ScatterLocData.data();
    m_algo->ScatterLocSize  = static_cast<int>(m_ScatterLocData.size());
    m_algo->RCS             = m_RCSData.empty()             ? nullptr : m_RCSData.data();
    m_algo->RCSSize         = static_cast<int>(m_RCSData.size());
    m_algo->RCS_Variance    = m_RCS_VarianceData.empty()    ? nullptr : m_RCS_VarianceData.data();
    m_algo->RCS_VarianceSize = static_cast<int>(m_RCS_VarianceData.size());
    m_algo->DurationTime    = m_DurationTimeData.empty()    ? nullptr : m_DurationTimeData.data();
    m_algo->DurationTimeSize = static_cast<int>(m_DurationTimeData.size());
    m_algo->Position_Initial = m_Position_InitialData.empty() ? nullptr : m_Position_InitialData.data();
    m_algo->Position_InitialSize = static_cast<int>(m_Position_InitialData.size());
    m_algo->Position_Initial_XYZ = m_Position_Initial_XYZData.empty() ? nullptr : m_Position_Initial_XYZData.data();
    m_algo->Position_Initial_XYZSize = static_cast<int>(m_Position_Initial_XYZData.size());
    m_algo->Velocity_Initial_XYZ = m_Velocity_Initial_XYZData.empty() ? nullptr : m_Velocity_Initial_XYZData.data();
    m_algo->Velocity_Initial_XYZSize = static_cast<int>(m_Velocity_Initial_XYZData.size());
    m_algo->Accelerate_XYZ = m_Accelerate_XYZData.empty()  ? nullptr : m_Accelerate_XYZData.data();
    m_algo->Accelerate_XYZSize = static_cast<int>(m_Accelerate_XYZData.size());
    m_algo->Jerk_XYZ       = m_Jerk_XYZData.empty()        ? nullptr : m_Jerk_XYZData.data();
    m_algo->Jerk_XYZSize   = static_cast<int>(m_Jerk_XYZData.size());

    m_algo->Velocity_Initial    = m_Velocity_Initial;
    m_algo->Accelerate_Initial  = m_Accelerate_Initial;
    m_algo->Accelerate_Variance = m_Accelerate_Variance;
    m_algo->TimeStep            = m_TimeStep;
}

// ============================================================================
// 枚举转换
// ============================================================================

RADAR_TargetScatterLocation::Trajectory_ModeEnum
RADAR_TargetScatterLocation_Block::ConvertStringToTrajectoryMode(const std::string& value)
{
    const std::string v = ToLowerCopy(TrimCopy(value));
    if (v == "eci_frame" || v == "0") return RADAR_TargetScatterLocation::ECI_Frame;
    if (v == "userdefined" || v == "user_defined" || v == "1") return RADAR_TargetScatterLocation::User_Defined;
    if (v == "simplexyz_frame" || v == "2") return RADAR_TargetScatterLocation::SimpleXYZ_Frame;
    return RADAR_TargetScatterLocation::ECI_Frame;
}

RADAR_TargetScatterLocation::IsRandomErrorEnum
RADAR_TargetScatterLocation_Block::ConvertStringToIsRandomError(const std::string& value)
{
    const std::string v = ToLowerCopy(TrimCopy(value));
    return (v == "true" || v == "1") ? RADAR_TargetScatterLocation::RandomError_true
                                     : RADAR_TargetScatterLocation::RandomError_false;
}

RADAR_TargetScatterLocation::IsRCSRandomEnum
RADAR_TargetScatterLocation_Block::ConvertStringToIsRCSRandom(const std::string& value)
{
    const std::string v = ToLowerCopy(TrimCopy(value));
    return (v == "true" || v == "1") ? RADAR_TargetScatterLocation::RCSRandom_true
                                     : RADAR_TargetScatterLocation::RCSRandom_false;
}

// ============================================================================
// 基本工具函数
// ============================================================================

double RADAR_TargetScatterLocation_Block::m_get_array_value(const std::vector<double>& data, int size, int idx, double defval)
{
    if (idx < 0 || idx >= size) return defval;
    return data[static_cast<std::size_t>(idx)];
}

RADAR_TargetScatterLocation_Block::Vec3
RADAR_TargetScatterLocation_Block::m_make_vec(double x, double y, double z)
{
    Vec3 v; v.x = x; v.y = y; v.z = z; return v;
}

RADAR_TargetScatterLocation_Block::Vec3
RADAR_TargetScatterLocation_Block::m_add(const Vec3& a, const Vec3& b)
{
    return m_make_vec(a.x + b.x, a.y + b.y, a.z + b.z);
}

RADAR_TargetScatterLocation_Block::Vec3
RADAR_TargetScatterLocation_Block::m_scale(const Vec3& a, double s)
{
    return m_make_vec(a.x * s, a.y * s, a.z * s);
}

double RADAR_TargetScatterLocation_Block::m_dot(const Vec3& a, const Vec3& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

RADAR_TargetScatterLocation_Block::Vec3
RADAR_TargetScatterLocation_Block::m_lla_to_ecef(double lonRad, double latRad, double h)
{
    const double a = kEarthSemiMajor;
    const double b = kEarthSemiMinor;
    const double a2 = a * a, b2 = b * b;
    const double e2 = 1.0 - b2 / a2;
    const double sinLat = std::sin(latRad), cosLat = std::cos(latRad);
    const double sinLon = std::sin(lonRad), cosLon = std::cos(lonRad);
    const double N = a / std::sqrt(1.0 - e2 * sinLat * sinLat);
    return m_make_vec((N + h) * cosLat * cosLon, (N + h) * cosLat * sinLon, (N * (1.0 - e2) + h) * sinLat);
}

RADAR_TargetScatterLocation_Block::Vec3
RADAR_TargetScatterLocation_Block::m_rotate_z(const Vec3& v, double theta)
{
    const double c = std::cos(theta), s = std::sin(theta);
    return m_make_vec(c * v.x - s * v.y, s * v.x + c * v.y, v.z);
}

// ============================================================================
// 算法辅助函数
// ============================================================================

bool RADAR_TargetScatterLocation_Block::validateParams() const
{
    const int n = (m_NumberOfTargetScatter > 0) ? m_NumberOfTargetScatter : 1;

    if (m_Trajectory_Mode == RADAR_TargetScatterLocation::User_Defined)
        return true;

    const int slSize = static_cast<int>(m_ScatterLocData.size());
    if (slSize != 3 * n) {
        std::cerr << "RADAR_TargetScatterLocation: ScatterLoc size should be 3*NumberOfTargetScatter" << std::endl;
        return false;
    }

    const int rcsSize = static_cast<int>(m_RCSData.size());
    if (rcsSize != n) {
        std::cerr << "RADAR_TargetScatterLocation: RCS size should equal NumberOfTargetScatter" << std::endl;
        return false;
    }

    return true;
}

void RADAR_TargetScatterLocation_Block::initEmptyUserSample()
{
    const int n = (m_NumberOfTargetScatter > 0) ? m_NumberOfTargetScatter : 1;
    m_lastUserSample.pos.assign(static_cast<std::size_t>(n), m_make_vec(0.0, 0.0, 0.0));
    m_lastUserSample.rcs.assign(static_cast<std::size_t>(n), 0.0);
}

bool RADAR_TargetScatterLocation_Block::loadUserFile()
{
    m_userPath.clear();
    m_userPathIndex = 0;
    initEmptyUserSample();

    const int n = (m_NumberOfTargetScatter > 0) ? m_NumberOfTargetScatter : 1;

    if (m_FileName.empty()) return true;

    std::ifstream fin(m_FileName);
    if (!fin) return true;

    std::string line;
    while (std::getline(fin, line)) {
        std::istringstream iss(line);
        UserSample sample;
        sample.pos.reserve(static_cast<std::size_t>(n));
        sample.rcs.reserve(static_cast<std::size_t>(n));

        bool ok = true;
        for (int i = 0; i < n; ++i) {
            double x = 0.0, y = 0.0, z = 0.0, r = 0.0;
            if (!(iss >> x >> y >> z >> r)) { ok = false; break; }
            sample.pos.push_back(m_make_vec(x, y, z));
            sample.rcs.push_back(r);
        }

        if (ok && static_cast<int>(sample.pos.size()) == n)
            m_userPath.push_back(sample);
    }

    if (!m_userPath.empty())
        m_lastUserSample = m_userPath.front();

    return true;
}

RADAR_TargetScatterLocation_Block::Vec3
RADAR_TargetScatterLocation_Block::getScatterLoc(int idx) const
{
    const int s = static_cast<int>(m_ScatterLocData.size());
    return m_make_vec(
        m_get_array_value(m_ScatterLocData, s, 3 * idx + 0, 0.0),
        m_get_array_value(m_ScatterLocData, s, 3 * idx + 1, 0.0),
        m_get_array_value(m_ScatterLocData, s, 3 * idx + 2, 0.0));
}

double RADAR_TargetScatterLocation_Block::getRcsBase(int idx) const
{
    return m_get_array_value(m_RCSData, static_cast<int>(m_RCSData.size()), idx, 1.0);
}

double RADAR_TargetScatterLocation_Block::baseRandomPosition(unsigned long long k) const
{
    static const double table[] = {
        0.205548047910945, 0.11099594587191, 0.57205602872446,
        0.586286278195218, -0.182779648757732, 0.00569209978830308,
        -0.0411096095821889, -0.409198729225788, 0.113209540234028,
        0.00505964425626941, -0.343107126128269, 0.334885204211831,
        0.0986630629972534, -0.03699864862397, -0.179617371097564,
        -0.49900741477457, -0.491417948390166, -0.668821725125612,
        -0.877215822930708, -0.65806998108104, -0.0610319588412497,
        0.330458015487596, 0.317808904846922, -0.576483217448695,
        -0.811124219833189, -0.453786844234162, -0.985998174440501,
        -1.24783476470244, -1.45749377357161, -1.84771883683638,
        -1.56754103614547, -1.92045122302026, -1.20040059979992,
        -1.35124124418995, -1.74273121851879, -1.83443727066368,
        -1.97168012111498, -2.62563914123781, -3.62586756514906,
        -4.13531049620219, -3.23817232401242, -3.81307440263103,
        -2.88178363171144, -2.93965331289253, -2.9523024235332,
        -2.55670148824613, -1.93341656142695, -0.873737317504523,
        -1.42903327463009, -1.99729457016235
    };
    const unsigned long long nTable = sizeof(table) / sizeof(table[0]);
    if (k < nTable) return table[k];

    unsigned long long x = 1469598103934665603ULL ^ (k + 0x9e3779b97f4a7c15ULL);
    x ^= x >> 12; x ^= x << 25; x ^= x >> 27;
    const unsigned long long y = x * 2685821657736338717ULL;
    const double u = (static_cast<double>((y >> 11) & 0x1FFFFFFFFFFFFFULL) + 1.0) / 9007199254740992.0;
    return 2.0 * u - 1.0;
}

double RADAR_TargetScatterLocation_Block::baseRandomRcs(unsigned long long blockIndex, int scatterIndex) const
{
    static const double table[] = {
        0.41125, -0.18900, 0.92200, -0.75600, 0.33700,
        -0.50800, 0.61400, -0.27100, 0.14300, -0.43600
    };
    const unsigned long long nTable = sizeof(table) / sizeof(table[0]);
    if (blockIndex < nTable) return table[blockIndex];

    unsigned long long x = 1099511628211ULL * static_cast<unsigned long long>(scatterIndex + 1);
    x ^= (blockIndex + 0x9e3779b97f4a7c15ULL);
    x ^= x >> 12; x ^= x << 25; x ^= x >> 27;
    const unsigned long long y = x * 2685821657736338717ULL;
    const double u = (static_cast<double>((y >> 11) & 0x1FFFFFFFFFFFFFULL) + 1.0) / 9007199254740992.0;
    return 2.0 * u - 1.0;
}

double RADAR_TargetScatterLocation_Block::calcScatterRcs(int idx, unsigned long long k) const
{
    const double base = getRcsBase(idx);

    if (m_IsRCSRandom != RADAR_TargetScatterLocation::RCSRandom_true)
        return base;

    const int dtSize = static_cast<int>(m_DurationTimeData.size());
    if (idx < 0 || idx >= dtSize) return base;

    const int rvSize = static_cast<int>(m_RCS_VarianceData.size());
    if (idx >= rvSize) return base;

    const double variance = m_RCS_VarianceData[static_cast<std::size_t>(idx)];
    if (variance == 0.0) return base;

    const double dur = m_DurationTimeData[static_cast<std::size_t>(idx)];
    if (dur <= 0.0 || m_TimeStep <= 0.0) return base;

    long long durSamples = static_cast<long long>(std::floor(dur / m_TimeStep + 1.0e-12));
    if (durSamples < 1) durSamples = 1;

    const unsigned long long holdSamples = static_cast<unsigned long long>(durSamples + 1);
    if (k < holdSamples) return base;

    const unsigned long long blockIndex = (k - holdSamples) / static_cast<unsigned long long>(durSamples);
    return base + variance * baseRandomRcs(blockIndex, idx);
}

// ============================================================================
// Setup
// ============================================================================

bool RADAR_TargetScatterLocation_Block::Setup()
{
    Block::Setup();

    m_inputBuffer.clear();
    while (!m_outputQueue.empty()) m_outputQueue.pop();

    if (m_NumberOfTargetScatter <= 0) m_NumberOfTargetScatter = 1;
    m_sampleIndex      = 0;
    m_motionAccumEcef  = m_make_vec(0.0, 0.0, 0.0);
    m_userPathIndex    = 0;

    if (!validateParams()) return false;

    // 初始化 ECI 模式常量
    const int sPos = static_cast<int>(m_Position_InitialData.size());
    const double lonDeg = m_get_array_value(m_Position_InitialData, sPos, 0, 0.0);
    const double latDeg = m_get_array_value(m_Position_InitialData, sPos, 1, 0.0);
    const double h      = m_get_array_value(m_Position_InitialData, sPos, 2, 0.0);

    m_lonRad = lonDeg * kDegToRad;
    m_latRad = latDeg * kDegToRad;

    m_p0Ecef = m_lla_to_ecef(m_lonRad, m_latRad, h);

    const double sinLon = std::sin(m_lonRad), cosLon = std::cos(m_lonRad);
    const double sinLat = std::sin(m_latRad), cosLat = std::cos(m_latRad);
    m_eastEcef  = m_make_vec(-sinLon, cosLon, 0.0);
    m_northEcef = m_make_vec(-sinLat * cosLon, -sinLat * sinLon, cosLat);
    m_upEcef    = m_make_vec(cosLat * cosLon, cosLat * sinLon, sinLat);

    loadUserFile();

    return true;
}

// ============================================================================
// Run — 双模式分发
// ============================================================================

bool RADAR_TargetScatterLocation_Block::Run()
{
    if (IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

// ============================================================================
// DataStreamRun — 数据流模式
// ============================================================================

bool RADAR_TargetScatterLocation_Block::DataStreamRun()
{
    // 读取输入（Roll/Pitch/Yaw 为可选输入端口）
    double rollDeg = 0.0, pitchDeg = 0.0, yawDeg = 0.0;

    {
        auto d = ReadInputData<double>(GetInputPortName(0));
        if (!d.empty()) rollDeg = d[0];
    }
    {
        auto d = ReadInputData<double>(GetInputPortName(1));
        if (!d.empty()) pitchDeg = d[0];
    }
    {
        auto d = ReadInputData<double>(GetInputPortName(2));
        if (!d.empty()) yawDeg = d[0];
    }

    const int n = (m_NumberOfTargetScatter > 0) ? m_NumberOfTargetScatter : 1;

    // ===== User_Defined 模式 =====
    if (m_Trajectory_Mode == RADAR_TargetScatterLocation::User_Defined) {
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

        std::vector<SystemVueModelBuilder::DoubleMatrix> posOut(static_cast<std::size_t>(n));
        std::vector<double> rcsOut(static_cast<std::size_t>(n));
        for (int i = 0; i < n; ++i) {
            SystemVueModelBuilder::DoubleMatrix pm(3, 1);
            pm(0, 0) = (i < static_cast<int>(out.pos.size())) ? out.pos[static_cast<std::size_t>(i)].x : 0.0;
            pm(1, 0) = (i < static_cast<int>(out.pos.size())) ? out.pos[static_cast<std::size_t>(i)].y : 0.0;
            pm(2, 0) = (i < static_cast<int>(out.pos.size())) ? out.pos[static_cast<std::size_t>(i)].z : 0.0;
            posOut[i] = pm;
            rcsOut[i] = (i < static_cast<int>(out.rcs.size())) ? out.rcs[static_cast<std::size_t>(i)] : 0.0;
        }
        WriteOutputData(GetOutputPortName(0), posOut);
        WriteOutputData(GetOutputPortName(1), rcsOut);
        ++m_sampleIndex;
        return true;
    }

    // ===== SimpleXYZ_Frame 模式 =====
    if (m_Trajectory_Mode == RADAR_TargetScatterLocation::SimpleXYZ_Frame) {
        const double k = static_cast<double>(m_sampleIndex);
        const double t = k * m_TimeStep;
        const double t2 = t * t;
        const double t3 = t2 * t;

        const int sPos  = static_cast<int>(m_Position_Initial_XYZData.size());
        const int sVel  = static_cast<int>(m_Velocity_Initial_XYZData.size());
        const int sAcc  = static_cast<int>(m_Accelerate_XYZData.size());
        const int sJerk = static_cast<int>(m_Jerk_XYZData.size());

        const double x0 = m_get_array_value(m_Position_Initial_XYZData, sPos, 0, 0.0);
        const double y0 = m_get_array_value(m_Position_Initial_XYZData, sPos, 1, 20e3);
        const double z0 = m_get_array_value(m_Position_Initial_XYZData, sPos, 2, 5e3);
        const double vx = m_get_array_value(m_Velocity_Initial_XYZData, sVel, 0, 0.0);
        const double vy = m_get_array_value(m_Velocity_Initial_XYZData, sVel, 1, 0.0);
        const double vz = m_get_array_value(m_Velocity_Initial_XYZData, sVel, 2, 0.0);
        const double ax = m_get_array_value(m_Accelerate_XYZData, sAcc, 0, 0.0);
        const double ay = m_get_array_value(m_Accelerate_XYZData, sAcc, 1, 0.0);
        const double az = m_get_array_value(m_Accelerate_XYZData, sAcc, 2, 0.0);
        const double jx = m_get_array_value(m_Jerk_XYZData, sJerk, 0, 0.0);
        const double jy = m_get_array_value(m_Jerk_XYZData, sJerk, 1, 0.0);
        const double jz = m_get_array_value(m_Jerk_XYZData, sJerk, 2, 0.0);

        const Vec3 center = m_make_vec(
            x0 + vx * t + 0.5 * ax * t2 + jx * t3 / 3.0,
            y0 + vy * t + 0.5 * ay * t2 + jy * t3 / 3.0,
            z0 + vz * t + 0.5 * az * t2 + jz * t3 / 3.0);

        std::vector<SystemVueModelBuilder::DoubleMatrix> posOut(static_cast<std::size_t>(n));
        std::vector<double> rcsOut(static_cast<std::size_t>(n));
        for (int i = 0; i < n; ++i) {
            Vec3 sc = getScatterLoc(i);
            Vec3 pos = m_add(center, sc);

            SystemVueModelBuilder::DoubleMatrix pm(3, 1);
            pm(0, 0) = pos.x;
            pm(1, 0) = pos.y;
            pm(2, 0) = pos.z;
            posOut[i] = pm;
            rcsOut[i] = calcScatterRcs(i, m_sampleIndex);
        }

        WriteOutputData(GetOutputPortName(0), posOut);
        WriteOutputData(GetOutputPortName(1), rcsOut);
        ++m_sampleIndex;
        return true;
    }

    // ===== ECI_Frame 模式 =====
    const double roll  = rollDeg * kDegToRad;
    const double pitch = pitchDeg * kDegToRad;
    const double yaw   = yawDeg * kDegToRad;

    const double cosRoll = std::cos(roll),  sinRoll = std::sin(roll);
    const double cosPitch = std::cos(pitch), sinPitch = std::sin(pitch);
    const double cosYaw  = std::cos(yaw),   sinYaw = std::sin(yaw);

    const double dirEast  = cosPitch * sinYaw;
    const double dirNorth = cosPitch * cosYaw;
    const double dirUp    = sinPitch;

    Vec3 dirEcef = m_add(
        m_add(m_scale(m_eastEcef, dirEast), m_scale(m_northEcef, dirNorth)),
        m_scale(m_upEcef, dirUp));

    const double norm2 = m_dot(dirEcef, dirEcef);
    if (norm2 > 0.0) dirEcef = m_scale(dirEcef, 1.0 / std::sqrt(norm2));

    const double stepDistance = m_Velocity_Initial * m_TimeStep
        + 0.5 * m_Accelerate_Initial * m_TimeStep * m_TimeStep;
    m_motionAccumEcef = m_add(m_motionAccumEcef, m_scale(dirEcef, stepDistance));

    double randomDistance = 0.0;
    if (m_IsRandomError == RADAR_TargetScatterLocation::RandomError_true && m_Accelerate_Variance > 0.0) {
        randomDistance = std::sqrt(m_Accelerate_Variance) * baseRandomPosition(m_sampleIndex);
    }

    const Vec3 centerMovedEcef = m_add(m_p0Ecef,
        m_add(m_motionAccumEcef, m_scale(dirEcef, randomDistance)));

    const double theta = kEarthRotRate * static_cast<double>(m_sampleIndex) * m_TimeStep;

    std::vector<SystemVueModelBuilder::DoubleMatrix> posOut(static_cast<std::size_t>(n));
    std::vector<double> rcsOut(static_cast<std::size_t>(n));

    for (int i = 0; i < n; ++i) {
        Vec3 sc = getScatterLoc(i);
        const double sx = sc.x, sy = sc.y, sz = sc.z;

        // body frame -> NED frame 旋转
        const double nLoc = cosPitch * cosYaw * sx
            + (sinRoll * sinPitch * cosYaw - cosRoll * sinYaw) * sy
            + (cosRoll * sinPitch * cosYaw + sinRoll * sinYaw) * sz;
        const double eLoc = cosPitch * sinYaw * sx
            + (sinRoll * sinPitch * sinYaw + cosRoll * cosYaw) * sy
            + (cosRoll * sinPitch * sinYaw - sinRoll * cosYaw) * sz;
        const double dLoc = -sinPitch * sx
            + sinRoll * cosPitch * sy
            + cosRoll * cosPitch * sz;

        // NED -> ECEF（N→northEcef, E→eastEcef, D→-upEcef）
        Vec3 scatterOffsetEcef = m_add(
            m_add(m_scale(m_northEcef, nLoc), m_scale(m_eastEcef, eLoc)),
            m_scale(m_upEcef, -dLoc));

        Vec3 scatterEcef = m_add(centerMovedEcef, scatterOffsetEcef);
        Vec3 pos = m_rotate_z(scatterEcef, theta);

        SystemVueModelBuilder::DoubleMatrix pm(3, 1);
        pm(0, 0) = pos.x;
        pm(1, 0) = pos.y;
        pm(2, 0) = pos.z;
        posOut[i] = pm;
        rcsOut[i] = calcScatterRcs(i, m_sampleIndex);
    }

    WriteOutputData(GetOutputPortName(0), posOut);
    WriteOutputData(GetOutputPortName(1), rcsOut);

    ++m_sampleIndex;
    return true;
}

// ============================================================================
// TimeDrivenRun — 变步长模式
// ============================================================================

bool RADAR_TargetScatterLocation_Block::TimeDrivenRun()
{
    // 累积输入
    {
        auto d0 = ReadInputData<double>(GetInputPortName(0));
        auto d1 = ReadInputData<double>(GetInputPortName(1));
        auto d2 = ReadInputData<double>(GetInputPortName(2));

        if (!d0.empty() || !d1.empty() || !d2.empty()) {
            InputSnapshot in;
            in.roll  = d0.empty() ? 0.0 : d0[0];
            in.pitch = d1.empty() ? 0.0 : d1[0];
            in.yaw   = d2.empty() ? 0.0 : d2[0];
            m_inputBuffer.push_back(in);
        }
    }

    // 处理所有累积输入 → 入队
    while (!m_inputBuffer.empty()) {
        InputSnapshot in = m_inputBuffer.front();
        m_inputBuffer.erase(m_inputBuffer.begin());

        const int n = (m_NumberOfTargetScatter > 0) ? m_NumberOfTargetScatter : 1;

        OutputFrame out;
        out.pos.resize(static_cast<std::size_t>(n));
        out.rcs.resize(static_cast<std::size_t>(n));

        // User_Defined
        if (m_Trajectory_Mode == RADAR_TargetScatterLocation::User_Defined) {
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
            }
            ++m_sampleIndex;
            m_outputQueue.push(out);
            continue;
        }

        // SimpleXYZ_Frame
        if (m_Trajectory_Mode == RADAR_TargetScatterLocation::SimpleXYZ_Frame) {
            const double k = static_cast<double>(m_sampleIndex);
            const double t = k * m_TimeStep;
            const double t2 = t * t;
            const double t3 = t2 * t;

            const int sPos  = static_cast<int>(m_Position_Initial_XYZData.size());
            const int sVel  = static_cast<int>(m_Velocity_Initial_XYZData.size());
            const int sAcc  = static_cast<int>(m_Accelerate_XYZData.size());
            const int sJerk = static_cast<int>(m_Jerk_XYZData.size());

            const double x0 = m_get_array_value(m_Position_Initial_XYZData, sPos, 0, 0.0);
            const double y0 = m_get_array_value(m_Position_Initial_XYZData, sPos, 1, 20e3);
            const double z0 = m_get_array_value(m_Position_Initial_XYZData, sPos, 2, 5e3);
            const double vx = m_get_array_value(m_Velocity_Initial_XYZData, sVel, 0, 0.0);
            const double vy = m_get_array_value(m_Velocity_Initial_XYZData, sVel, 1, 0.0);
            const double vz = m_get_array_value(m_Velocity_Initial_XYZData, sVel, 2, 0.0);
            const double ax = m_get_array_value(m_Accelerate_XYZData, sAcc, 0, 0.0);
            const double ay = m_get_array_value(m_Accelerate_XYZData, sAcc, 1, 0.0);
            const double az = m_get_array_value(m_Accelerate_XYZData, sAcc, 2, 0.0);
            const double jx = m_get_array_value(m_Jerk_XYZData, sJerk, 0, 0.0);
            const double jy = m_get_array_value(m_Jerk_XYZData, sJerk, 1, 0.0);
            const double jz = m_get_array_value(m_Jerk_XYZData, sJerk, 2, 0.0);

            const Vec3 center = m_make_vec(
                x0 + vx * t + 0.5 * ax * t2 + jx * t3 / 3.0,
                y0 + vy * t + 0.5 * ay * t2 + jy * t3 / 3.0,
                z0 + vz * t + 0.5 * az * t2 + jz * t3 / 3.0);

            for (int i = 0; i < n; ++i) {
                Vec3 sc = getScatterLoc(i);
                Vec3 pos = m_add(center, sc);

                SystemVueModelBuilder::DoubleMatrix pm(3, 1);
                pm(0, 0) = pos.x; pm(1, 0) = pos.y; pm(2, 0) = pos.z;
                out.pos[i] = pm;
                out.rcs[i] = calcScatterRcs(i, m_sampleIndex);
            }
            ++m_sampleIndex;
            m_outputQueue.push(out);
            continue;
        }

        // ECI_Frame
        const double roll  = in.roll * kDegToRad;
        const double pitch = in.pitch * kDegToRad;
        const double yaw   = in.yaw * kDegToRad;

        const double cosRoll = std::cos(roll),  sinRoll = std::sin(roll);
        const double cosPitch = std::cos(pitch), sinPitch = std::sin(pitch);
        const double cosYaw  = std::cos(yaw),   sinYaw = std::sin(yaw);

        const double dirEast  = cosPitch * sinYaw;
        const double dirNorth = cosPitch * cosYaw;
        const double dirUp    = sinPitch;

        Vec3 dirEcef = m_add(
            m_add(m_scale(m_eastEcef, dirEast), m_scale(m_northEcef, dirNorth)),
            m_scale(m_upEcef, dirUp));

        const double norm2 = m_dot(dirEcef, dirEcef);
        if (norm2 > 0.0) dirEcef = m_scale(dirEcef, 1.0 / std::sqrt(norm2));

        const double stepDistance = m_Velocity_Initial * m_TimeStep
            + 0.5 * m_Accelerate_Initial * m_TimeStep * m_TimeStep;
        m_motionAccumEcef = m_add(m_motionAccumEcef, m_scale(dirEcef, stepDistance));

        double randomDistance = 0.0;
        if (m_IsRandomError == RADAR_TargetScatterLocation::RandomError_true && m_Accelerate_Variance > 0.0) {
            randomDistance = std::sqrt(m_Accelerate_Variance) * baseRandomPosition(m_sampleIndex);
        }

        const Vec3 centerMovedEcef = m_add(m_p0Ecef,
            m_add(m_motionAccumEcef, m_scale(dirEcef, randomDistance)));

        const double theta = kEarthRotRate * static_cast<double>(m_sampleIndex) * m_TimeStep;

        for (int i = 0; i < n; ++i) {
            Vec3 sc = getScatterLoc(i);
            const double sx = sc.x, sy = sc.y, sz = sc.z;

            const double nLoc = cosPitch * cosYaw * sx
                + (sinRoll * sinPitch * cosYaw - cosRoll * sinYaw) * sy
                + (cosRoll * sinPitch * cosYaw + sinRoll * sinYaw) * sz;
            const double eLoc = cosPitch * sinYaw * sx
                + (sinRoll * sinPitch * sinYaw + cosRoll * cosYaw) * sy
                + (cosRoll * sinPitch * sinYaw - sinRoll * cosYaw) * sz;
            const double dLoc = -sinPitch * sx
                + sinRoll * cosPitch * sy
                + cosRoll * cosPitch * sz;

            Vec3 scatterOffsetEcef = m_add(
                m_add(m_scale(m_northEcef, nLoc), m_scale(m_eastEcef, eLoc)),
                m_scale(m_upEcef, -dLoc));

            Vec3 scatterEcef = m_add(centerMovedEcef, scatterOffsetEcef);
            Vec3 pos = m_rotate_z(scatterEcef, theta);

            SystemVueModelBuilder::DoubleMatrix pm(3, 1);
            pm(0, 0) = pos.x; pm(1, 0) = pos.y; pm(2, 0) = pos.z;
            out.pos[i] = pm;
            out.rcs[i] = calcScatterRcs(i, m_sampleIndex);
        }

        ++m_sampleIndex;
        m_outputQueue.push(out);
    }

    // 出队写入
    if (!m_outputQueue.empty()) {
        OutputFrame out = m_outputQueue.front();
        m_outputQueue.pop();
        WriteOutputData(GetOutputPortName(0), out.pos);
        WriteOutputData(GetOutputPortName(1), out.rcs);
    }

    return true;
}

// ============================================================================
// Initialize
// ============================================================================

bool RADAR_TargetScatterLocation_Block::Initialize()
{
    SetBlockType(Block::BlockType::SOURCE);

    m_algo = std::make_unique<RADAR_TargetScatterLocation>();

    SetDefaultParameters();

    // 解析参数
    try { m_Trajectory_Mode = ConvertStringToTrajectoryMode(getParameter("Trajectory_Mode").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Trajectory_Mode', using default value."); }
    try { m_NumberOfTargetScatter = std::stoi(getParameter("NumberOfTargetScatter").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'NumberOfTargetScatter', using default value."); }
    try { m_FileName = getParameter("FileName").Value; } catch (...) { LOG_WARN("Failed to parse parameter 'FileName', using default value."); }

    try { m_ScatterLocData = ParseStringToDoubleVector(getParameter("ScatterLoc").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'ScatterLoc', using default value."); }
    try { m_RCSData = ParseStringToDoubleVector(getParameter("RCS").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'RCS', using default value."); }
    try { m_RCS_VarianceData = ParseStringToDoubleVector(getParameter("RCS_Variance").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'RCS_Variance', using default value."); }
    try { m_DurationTimeData = ParseStringToDoubleVector(getParameter("DurationTime").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'DurationTime', using default value."); }
    try { m_Position_InitialData = ParseStringToDoubleVector(getParameter("Position_Initial").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Position_Initial', using default value."); }
    try { m_Position_Initial_XYZData = ParseStringToDoubleVector(getParameter("Position_Initial_XYZ").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Position_Initial_XYZ', using default value."); }
    try { m_Velocity_Initial_XYZData = ParseStringToDoubleVector(getParameter("Velocity_Initial_XYZ").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Velocity_Initial_XYZ', using default value."); }
    try { m_Accelerate_XYZData = ParseStringToDoubleVector(getParameter("Accelerate_XYZ").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Accelerate_XYZ', using default value."); }
    try { m_Jerk_XYZData = ParseStringToDoubleVector(getParameter("Jerk_XYZ").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Jerk_XYZ', using default value."); }

    try { m_IsRandomError = ConvertStringToIsRandomError(getParameter("IsRandomError").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'IsRandomError', using default value."); }
    try { m_IsRCSRandom = ConvertStringToIsRCSRandom(getParameter("IsRCSRandom").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'IsRCSRandom', using default value."); }

    try { m_Velocity_Initial = std::stod(getParameter("Velocity_Initial").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Velocity_Initial', using default value."); }
    try { m_Accelerate_Initial = std::stod(getParameter("Accelerate_Initial").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Accelerate_Initial', using default value."); }
    try { m_Accelerate_Variance = std::stod(getParameter("Accelerate_Variance").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Accelerate_Variance', using default value."); }
    try { m_TimeStep = std::stod(getParameter("TimeStep").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'TimeStep', using default value."); }

    SetAlgoParameters();

    if (!m_algo->Setup()) return false;

    // 输入端口（Roll/Pitch/Yaw 为可选）
    AddInputPort("Roll",  m_algo->Roll,  1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddInputPort("Pitch", m_algo->Pitch, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddInputPort("Yaw",   m_algo->Yaw,   1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);

    // 输出端口（bus 类型）
    AddOutputPort("Pos",       m_algo->Pos,       m_NumberOfTargetScatter, Block::DataType::MATRIX_DOUBLE_BUS);
    AddOutputPort("ScatterRCS", m_algo->ScatterRCS, m_NumberOfTargetScatter, Block::DataType::DOUBLE_BUS);

    return true;
}
