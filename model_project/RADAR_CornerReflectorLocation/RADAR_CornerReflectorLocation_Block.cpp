#include "RADAR_CornerReflectorLocation_Block.h"

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
static const double kRadToDeg        = 180.0 / kPi;
static const double kSpeedOfLight    = 299792458.0;
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

RADAR_CornerReflectorLocation_Block::RADAR_CornerReflectorLocation_Block(const std::string& name)
    : Block(name)
    , m_Trajectory_Mode(RADAR_CornerReflectorLocation::SimpleXYZ_Frame)
    , m_Motion_Mode(RADAR_CornerReflectorLocation::Fixed_Mode)
    , m_NumberOfCornerReflector(1)
    , m_ReflectorType(RADAR_CornerReflectorLocation::Triangular_Trihedral)
    , m_RCS_Model(RADAR_CornerReflectorLocation::PeakOnly)
    , m_RCS_OutputUnit(RADAR_CornerReflectorLocation::Linear_m2)
    , m_CarrierFreq(10e9)
    , m_BoresightHalfAngle(30.0)
    , m_RCS_Floor(0.0)
    , m_Velocity_Initial(0.0)
    , m_Accelerate_Initial(0.0)
    , m_TimeStep(1e-9)
    , m_sampleIndex(0)
    , m_p0Ecef(m_make_vec(0.0, 0.0, 0.0))
    , m_eastEcef(m_make_vec(0.0, 1.0, 0.0))
    , m_northEcef(m_make_vec(0.0, 0.0, 1.0))
    , m_upEcef(m_make_vec(1.0, 0.0, 0.0))
    , m_lonRad(0.0)
    , m_latRad(0.0)
    , m_userPathIndex(0)
{
    m_init_empty_user_sample();
}

// ============================================================================
// SetDefaultParameters
// ============================================================================

void RADAR_CornerReflectorLocation_Block::SetDefaultParameters()
{
    m_Trajectory_Mode          = RADAR_CornerReflectorLocation::SimpleXYZ_Frame;
    m_Motion_Mode              = RADAR_CornerReflectorLocation::Fixed_Mode;
    m_NumberOfCornerReflector  = 1;
    m_ReflectorType            = RADAR_CornerReflectorLocation::Triangular_Trihedral;
    m_RCS_Model                = RADAR_CornerReflectorLocation::PeakOnly;
    m_RCS_OutputUnit           = RADAR_CornerReflectorLocation::Linear_m2;
    m_FileName                 = "";
    m_CornerLocData            = { 0.0, 0.0, 0.0 };
    m_EdgeLengthData           = { 1.0 };
    m_EfficiencyData           = { 1.0 };
    m_CornerRollOffsetData     = { 0.0 };
    m_CornerPitchOffsetData    = { 0.0 };
    m_CornerYawOffsetData      = { 0.0 };
    m_PhaseCenterOffsetData    = { 0.0, 0.0, 0.0 };
    m_Radar_Position_XYZData   = { 0.0, 0.0, 0.0 };
    m_Position_InitialData     = { 0.0, 0.0, 0.0 };
    m_Position_Initial_XYZData = { 0.0, 0.0, 0.0 };
    m_Velocity_Initial_XYZData = { 0.0, 0.0, 0.0 };
    m_Accelerate_XYZData       = { 0.0, 0.0, 0.0 };
    m_Jerk_XYZData             = { 0.0, 0.0, 0.0 };
    m_CarrierFreq              = 10e9;
    m_BoresightHalfAngle       = 30.0;
    m_RCS_Floor                = 0.0;
    m_Velocity_Initial         = 0.0;
    m_Accelerate_Initial       = 0.0;
    m_TimeStep                 = 1e-9;
}

// ============================================================================
// SetAlgoParameters
// ============================================================================

void RADAR_CornerReflectorLocation_Block::SetAlgoParameters()
{
    if (!m_algo) return;

    m_algo->Trajectory_Mode          = m_Trajectory_Mode;
    m_algo->Motion_Mode              = m_Motion_Mode;
    m_algo->NumberOfCornerReflector  = m_NumberOfCornerReflector;
    m_algo->ReflectorType            = m_ReflectorType;
    m_algo->RCS_Model                = m_RCS_Model;
    m_algo->RCS_OutputUnit           = m_RCS_OutputUnit;

    m_algo->FileName = const_cast<char*>(m_FileName.c_str());

    m_algo->CornerLoc              = m_CornerLocData.empty()          ? nullptr : m_CornerLocData.data();
    m_algo->CornerLocSize          = static_cast<int>(m_CornerLocData.size());
    m_algo->EdgeLength             = m_EdgeLengthData.empty()         ? nullptr : m_EdgeLengthData.data();
    m_algo->EdgeLengthSize         = static_cast<int>(m_EdgeLengthData.size());
    m_algo->Efficiency             = m_EfficiencyData.empty()         ? nullptr : m_EfficiencyData.data();
    m_algo->EfficiencySize         = static_cast<int>(m_EfficiencyData.size());
    m_algo->CornerRollOffset       = m_CornerRollOffsetData.empty()   ? nullptr : m_CornerRollOffsetData.data();
    m_algo->CornerRollOffsetSize   = static_cast<int>(m_CornerRollOffsetData.size());
    m_algo->CornerPitchOffset      = m_CornerPitchOffsetData.empty()  ? nullptr : m_CornerPitchOffsetData.data();
    m_algo->CornerPitchOffsetSize  = static_cast<int>(m_CornerPitchOffsetData.size());
    m_algo->CornerYawOffset        = m_CornerYawOffsetData.empty()    ? nullptr : m_CornerYawOffsetData.data();
    m_algo->CornerYawOffsetSize    = static_cast<int>(m_CornerYawOffsetData.size());
    m_algo->PhaseCenterOffset      = m_PhaseCenterOffsetData.empty()  ? nullptr : m_PhaseCenterOffsetData.data();
    m_algo->PhaseCenterOffsetSize  = static_cast<int>(m_PhaseCenterOffsetData.size());
    m_algo->Radar_Position_XYZ     = m_Radar_Position_XYZData.empty() ? nullptr : m_Radar_Position_XYZData.data();
    m_algo->Radar_Position_XYZSize = static_cast<int>(m_Radar_Position_XYZData.size());
    m_algo->Position_Initial       = m_Position_InitialData.empty()   ? nullptr : m_Position_InitialData.data();
    m_algo->Position_InitialSize   = static_cast<int>(m_Position_InitialData.size());
    m_algo->Position_Initial_XYZ   = m_Position_Initial_XYZData.empty() ? nullptr : m_Position_Initial_XYZData.data();
    m_algo->Position_Initial_XYZSize = static_cast<int>(m_Position_Initial_XYZData.size());
    m_algo->Velocity_Initial_XYZ   = m_Velocity_Initial_XYZData.empty() ? nullptr : m_Velocity_Initial_XYZData.data();
    m_algo->Velocity_Initial_XYZSize = static_cast<int>(m_Velocity_Initial_XYZData.size());
    m_algo->Accelerate_XYZ         = m_Accelerate_XYZData.empty()     ? nullptr : m_Accelerate_XYZData.data();
    m_algo->Accelerate_XYZSize     = static_cast<int>(m_Accelerate_XYZData.size());
    m_algo->Jerk_XYZ               = m_Jerk_XYZData.empty()           ? nullptr : m_Jerk_XYZData.data();
    m_algo->Jerk_XYZSize           = static_cast<int>(m_Jerk_XYZData.size());

    m_algo->CarrierFreq         = m_CarrierFreq;
    m_algo->BoresightHalfAngle  = m_BoresightHalfAngle;
    m_algo->RCS_Floor           = m_RCS_Floor;
    m_algo->Velocity_Initial    = m_Velocity_Initial;
    m_algo->Accelerate_Initial  = m_Accelerate_Initial;
    m_algo->TimeStep            = m_TimeStep;
}

// ============================================================================
// ConvertStringTo — 5 个枚举
// ============================================================================

RADAR_CornerReflectorLocation::Trajectory_ModeEnum
RADAR_CornerReflectorLocation_Block::ConvertStringToTrajectoryMode(const std::string& value)
{
    const std::string v = ToLowerCopy(TrimCopy(value));
    if (v == "eci_frame"     || v == "0") return RADAR_CornerReflectorLocation::ECI_Frame;
    if (v == "userdefined"   || v == "user_defined" || v == "1") return RADAR_CornerReflectorLocation::User_Defined;
    if (v == "simplexyz_frame" || v == "2") return RADAR_CornerReflectorLocation::SimpleXYZ_Frame;
    return RADAR_CornerReflectorLocation::SimpleXYZ_Frame;
}

RADAR_CornerReflectorLocation::Motion_ModeEnum
RADAR_CornerReflectorLocation_Block::ConvertStringToMotionMode(const std::string& value)
{
    const std::string v = ToLowerCopy(TrimCopy(value));
    if (v == "fixed_mode"  || v == "0") return RADAR_CornerReflectorLocation::Fixed_Mode;
    if (v == "moving_mode" || v == "1") return RADAR_CornerReflectorLocation::Moving_Mode;
    return RADAR_CornerReflectorLocation::Fixed_Mode;
}

RADAR_CornerReflectorLocation::ReflectorTypeEnum
RADAR_CornerReflectorLocation_Block::ConvertStringToReflectorType(const std::string& value)
{
    const std::string v = ToLowerCopy(TrimCopy(value));
    if (v == "triangular_trihedral" || v == "0") return RADAR_CornerReflectorLocation::Triangular_Trihedral;
    if (v == "square_trihedral"     || v == "1") return RADAR_CornerReflectorLocation::Square_Trihedral;
    return RADAR_CornerReflectorLocation::Triangular_Trihedral;
}

RADAR_CornerReflectorLocation::RCSModelEnum
RADAR_CornerReflectorLocation_Block::ConvertStringToRCSModel(const std::string& value)
{
    const std::string v = ToLowerCopy(TrimCopy(value));
    if (v == "peakonly"      || v == "0") return RADAR_CornerReflectorLocation::PeakOnly;
    if (v == "boresightcone" || v == "1") return RADAR_CornerReflectorLocation::BoresightCone;
    return RADAR_CornerReflectorLocation::PeakOnly;
}

RADAR_CornerReflectorLocation::RCSOutputUnitEnum
RADAR_CornerReflectorLocation_Block::ConvertStringToRCSOutputUnit(const std::string& value)
{
    const std::string v = ToLowerCopy(TrimCopy(value));
    if (v == "linear_m2" || v == "0") return RADAR_CornerReflectorLocation::Linear_m2;
    if (v == "dbsm"       || v == "1") return RADAR_CornerReflectorLocation::dBsm;
    return RADAR_CornerReflectorLocation::Linear_m2;
}

// ============================================================================
// 基本工具函数
// ============================================================================

double RADAR_CornerReflectorLocation_Block::m_get_array_value(const std::vector<double>& data, int size, int idx, double defval)
{
    if (idx < 0 || idx >= size) return defval;
    return data[static_cast<std::size_t>(idx)];
}

double RADAR_CornerReflectorLocation_Block::m_get_array_broadcast(const std::vector<double>& data, int size, int idx, double defval)
{
    if (size <= 0) return defval;
    if (size == 1) return data[0];
    if (idx >= 0 && idx < size) return data[static_cast<std::size_t>(idx)];
    return defval;
}

RADAR_CornerReflectorLocation_Block::Vec3 RADAR_CornerReflectorLocation_Block::m_make_vec(double x, double y, double z)
{
    Vec3 v; v.x = x; v.y = y; v.z = z; return v;
}

RADAR_CornerReflectorLocation_Block::Vec3 RADAR_CornerReflectorLocation_Block::m_add(const Vec3& a, const Vec3& b)
{
    return m_make_vec(a.x + b.x, a.y + b.y, a.z + b.z);
}

RADAR_CornerReflectorLocation_Block::Vec3 RADAR_CornerReflectorLocation_Block::m_sub(const Vec3& a, const Vec3& b)
{
    return m_make_vec(a.x - b.x, a.y - b.y, a.z - b.z);
}

RADAR_CornerReflectorLocation_Block::Vec3 RADAR_CornerReflectorLocation_Block::m_scale(const Vec3& a, double s)
{
    return m_make_vec(a.x * s, a.y * s, a.z * s);
}

double RADAR_CornerReflectorLocation_Block::m_dot(const Vec3& a, const Vec3& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

double RADAR_CornerReflectorLocation_Block::m_norm(const Vec3& a)
{
    return std::sqrt(m_dot(a, a));
}

RADAR_CornerReflectorLocation_Block::Vec3 RADAR_CornerReflectorLocation_Block::m_normalize(const Vec3& a, const Vec3& fallback)
{
    const double n = m_norm(a);
    if (n <= 0.0) return fallback;
    return m_scale(a, 1.0 / n);
}

RADAR_CornerReflectorLocation_Block::Mat3 RADAR_CornerReflectorLocation_Block::m_identity_mat()
{
    Mat3 A;
    A.m[0][0] = 1.0; A.m[0][1] = 0.0; A.m[0][2] = 0.0;
    A.m[1][0] = 0.0; A.m[1][1] = 1.0; A.m[1][2] = 0.0;
    A.m[2][0] = 0.0; A.m[2][1] = 0.0; A.m[2][2] = 1.0;
    return A;
}

RADAR_CornerReflectorLocation_Block::Mat3 RADAR_CornerReflectorLocation_Block::m_mat_mul(const Mat3& A, const Mat3& B)
{
    Mat3 C;
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            C.m[r][c] = A.m[r][0] * B.m[0][c] + A.m[r][1] * B.m[1][c] + A.m[r][2] * B.m[2][c];
        }
    }
    return C;
}

RADAR_CornerReflectorLocation_Block::Vec3 RADAR_CornerReflectorLocation_Block::m_mat_vec(const Mat3& A, const Vec3& v)
{
    return m_make_vec(
        A.m[0][0] * v.x + A.m[0][1] * v.y + A.m[0][2] * v.z,
        A.m[1][0] * v.x + A.m[1][1] * v.y + A.m[1][2] * v.z,
        A.m[2][0] * v.x + A.m[2][1] * v.y + A.m[2][2] * v.z);
}

RADAR_CornerReflectorLocation_Block::Mat3 RADAR_CornerReflectorLocation_Block::m_transpose(const Mat3& A)
{
    Mat3 B;
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c)
            B.m[r][c] = A.m[c][r];
    return B;
}

RADAR_CornerReflectorLocation_Block::Mat3 RADAR_CornerReflectorLocation_Block::m_rot_x(double a)
{
    const double c = std::cos(a), s = std::sin(a);
    Mat3 R = m_identity_mat();
    R.m[1][1] = c;  R.m[1][2] = -s;
    R.m[2][1] = s;  R.m[2][2] = c;
    return R;
}

RADAR_CornerReflectorLocation_Block::Mat3 RADAR_CornerReflectorLocation_Block::m_rot_y(double a)
{
    const double c = std::cos(a), s = std::sin(a);
    Mat3 R = m_identity_mat();
    R.m[0][0] = c;  R.m[0][2] = s;
    R.m[2][0] = -s; R.m[2][2] = c;
    return R;
}

RADAR_CornerReflectorLocation_Block::Mat3 RADAR_CornerReflectorLocation_Block::m_rot_z(double a)
{
    const double c = std::cos(a), s = std::sin(a);
    Mat3 R = m_identity_mat();
    R.m[0][0] = c;  R.m[0][1] = -s;
    R.m[1][0] = s;  R.m[1][1] = c;
    return R;
}

RADAR_CornerReflectorLocation_Block::Mat3 RADAR_CornerReflectorLocation_Block::m_rpy_matrix(double rollRad, double pitchRad, double yawRad)
{
    return m_mat_mul(m_mat_mul(m_rot_z(yawRad), m_rot_y(pitchRad)), m_rot_x(rollRad));
}

RADAR_CornerReflectorLocation_Block::Vec3 RADAR_CornerReflectorLocation_Block::m_lla_to_ecef(double lonRad, double latRad, double h)
{
    const double a2 = kEarthSemiMajor * kEarthSemiMajor;
    const double b2 = kEarthSemiMinor * kEarthSemiMinor;
    const double e2 = 1.0 - b2 / a2;
    const double sinLat = std::sin(latRad), cosLat = std::cos(latRad);
    const double sinLon = std::sin(lonRad), cosLon = std::cos(lonRad);
    const double N = kEarthSemiMajor / std::sqrt(1.0 - e2 * sinLat * sinLat);
    return m_make_vec((N + h) * cosLat * cosLon, (N + h) * cosLat * sinLon, (N * (1.0 - e2) + h) * sinLat);
}

RADAR_CornerReflectorLocation_Block::Mat3 RADAR_CornerReflectorLocation_Block::m_ned_to_ecef_matrix(const Vec3& north, const Vec3& east, const Vec3& up)
{
    Mat3 M;
    M.m[0][0] = north.x; M.m[0][1] = east.x; M.m[0][2] = -up.x;
    M.m[1][0] = north.y; M.m[1][1] = east.y; M.m[1][2] = -up.y;
    M.m[2][0] = north.z; M.m[2][1] = east.z; M.m[2][2] = -up.z;
    return M;
}

RADAR_CornerReflectorLocation_Block::Mat3 RADAR_CornerReflectorLocation_Block::m_ecef_to_eci_matrix(double theta)
{
    return m_rot_z(theta);
}

double RADAR_CornerReflectorLocation_Block::m_safe_log10(double x)
{
    return std::log10((x > 1.0e-300) ? x : 1.0e-300);
}

double RADAR_CornerReflectorLocation_Block::m_clamp(double x, double lo, double hi)
{
    return std::max(lo, std::min(hi, x));
}

// ============================================================================
// 算法辅助函数
// ============================================================================

bool RADAR_CornerReflectorLocation_Block::m_validate_params() const
{
    const int n = (m_NumberOfCornerReflector > 0) ? m_NumberOfCornerReflector : 1;

    if (m_Trajectory_Mode == RADAR_CornerReflectorLocation::User_Defined)
        return true;

    const int clSize = static_cast<int>(m_CornerLocData.size());
    if (clSize != 0 && clSize != 3 * n) {
        std::cerr << "RADAR_CornerReflectorLocation: CornerLoc should have 3*NumberOfCornerReflector elements.\n";
        return false;
    }

    const int elSize = static_cast<int>(m_EdgeLengthData.size());
    if (elSize != 0 && !(elSize == 1 || elSize == n)) return false;

    const int efSize = static_cast<int>(m_EfficiencyData.size());
    if (!(efSize == 0 || efSize == 1 || efSize == n)) return false;

    const int crSize = static_cast<int>(m_CornerRollOffsetData.size());
    if (!(crSize == 0 || crSize == 1 || crSize == n)) return false;

    const int cpSize = static_cast<int>(m_CornerPitchOffsetData.size());
    if (!(cpSize == 0 || cpSize == 1 || cpSize == n)) return false;

    const int cySize = static_cast<int>(m_CornerYawOffsetData.size());
    if (!(cySize == 0 || cySize == 1 || cySize == n)) return false;

    const int pcSize = static_cast<int>(m_PhaseCenterOffsetData.size());
    if (!(pcSize == 0 || pcSize == 3 || pcSize == 3 * n)) return false;

    if (m_CarrierFreq <= 0.0) return false;

    return true;
}

void RADAR_CornerReflectorLocation_Block::m_init_empty_user_sample()
{
    const int n = (m_NumberOfCornerReflector > 0) ? m_NumberOfCornerReflector : 1;
    m_lastUserSample.pos.assign(static_cast<std::size_t>(n), m_make_vec(0.0, 0.0, 0.0));
    m_lastUserSample.rcs.assign(static_cast<std::size_t>(n), 0.0);
}

bool RADAR_CornerReflectorLocation_Block::m_load_user_file()
{
    m_userPath.clear();
    m_userPathIndex = 0;
    m_init_empty_user_sample();

    const int n = (m_NumberOfCornerReflector > 0) ? m_NumberOfCornerReflector : 1;

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

RADAR_CornerReflectorLocation_Block::Vec3 RADAR_CornerReflectorLocation_Block::m_get_corner_loc(int idx) const
{
    const int s = static_cast<int>(m_CornerLocData.size());
    return m_make_vec(
        m_get_array_value(m_CornerLocData, s, 3 * idx + 0, 0.0),
        m_get_array_value(m_CornerLocData, s, 3 * idx + 1, 0.0),
        m_get_array_value(m_CornerLocData, s, 3 * idx + 2, 0.0));
}

RADAR_CornerReflectorLocation_Block::Vec3 RADAR_CornerReflectorLocation_Block::m_get_phase_center_offset(int idx) const
{
    const int s = static_cast<int>(m_PhaseCenterOffsetData.size());
    if (s <= 0) return m_make_vec(0.0, 0.0, 0.0);
    if (s == 3) return m_make_vec(
        m_get_array_value(m_PhaseCenterOffsetData, s, 0, 0.0),
        m_get_array_value(m_PhaseCenterOffsetData, s, 1, 0.0),
        m_get_array_value(m_PhaseCenterOffsetData, s, 2, 0.0));
    return m_make_vec(
        m_get_array_value(m_PhaseCenterOffsetData, s, 3 * idx + 0, 0.0),
        m_get_array_value(m_PhaseCenterOffsetData, s, 3 * idx + 1, 0.0),
        m_get_array_value(m_PhaseCenterOffsetData, s, 3 * idx + 2, 0.0));
}

double RADAR_CornerReflectorLocation_Block::m_get_edge_length(int idx) const
{
    return m_get_array_broadcast(m_EdgeLengthData, static_cast<int>(m_EdgeLengthData.size()), idx, 1.0);
}

double RADAR_CornerReflectorLocation_Block::m_get_efficiency(int idx) const
{
    double eta = m_get_array_broadcast(m_EfficiencyData, static_cast<int>(m_EfficiencyData.size()), idx, 1.0);
    return (eta < 0.0) ? 0.0 : eta;
}

double RADAR_CornerReflectorLocation_Block::m_get_carrier_freq(double inputFreq)
{
    if (inputFreq > 0.0) return inputFreq;
    return m_CarrierFreq;
}

RADAR_CornerReflectorLocation_Block::Vec3 RADAR_CornerReflectorLocation_Block::m_get_radar_position(double rx, double ry, double rz)
{
    const int s = static_cast<int>(m_Radar_Position_XYZData.size());
    double x = m_get_array_value(m_Radar_Position_XYZData, s, 0, 0.0);
    double y = m_get_array_value(m_Radar_Position_XYZData, s, 1, 0.0);
    double z = m_get_array_value(m_Radar_Position_XYZData, s, 2, 0.0);
    return m_make_vec(rx, ry, rz);  // 输入端口值优先（始终连接）
}

double RADAR_CornerReflectorLocation_Block::m_peak_rcs_linear(int idx, double freqHz) const
{
    const double a = m_get_edge_length(idx);
    if (a <= 0.0 || freqHz <= 0.0) return 0.0;

    const double lambda = kSpeedOfLight / freqHz;
    const double a4 = a * a * a * a;

    double sigma = 0.0;
    if (m_ReflectorType == RADAR_CornerReflectorLocation::Triangular_Trihedral)
        sigma = 4.0 * kPi * a4 / (3.0 * lambda * lambda);
    else
        sigma = 12.0 * kPi * a4 / (lambda * lambda);

    sigma *= m_get_efficiency(idx);
    if (sigma < 0.0) sigma = 0.0;
    return sigma;
}

double RADAR_CornerReflectorLocation_Block::m_calc_corner_rcs(int idx,
    const Vec3& reflectorPos, const Mat3& bodyToGlobal, double freqHz,
    double radarX, double radarY, double radarZ)
{
    double sigma = m_peak_rcs_linear(idx, freqHz);

    if (m_RCS_Model == RADAR_CornerReflectorLocation::BoresightCone) {
        const Vec3 radarPos = m_make_vec(radarX, radarY, radarZ);
        const Vec3 uGlobal = m_normalize(m_sub(radarPos, reflectorPos), m_make_vec(1.0, 0.0, 0.0));
        const Mat3 globalToBody = m_transpose(bodyToGlobal);
        const Vec3 uBody = m_mat_vec(globalToBody, uGlobal);

        const bool inAperture = (uBody.x > 0.0 && uBody.y > 0.0 && uBody.z > 0.0);
        const Vec3 boresight = m_normalize(m_make_vec(1.0, 1.0, 1.0), m_make_vec(1.0, 0.0, 0.0));
        const Vec3 uBodyNorm = m_normalize(uBody, boresight);
        const double cosAng = m_clamp(m_dot(uBodyNorm, boresight), -1.0, 1.0);
        const double aspectDeg = std::acos(cosAng) * kRadToDeg;

        if (!inAperture || aspectDeg > m_BoresightHalfAngle)
            sigma = (m_RCS_Floor > 0.0) ? m_RCS_Floor : 0.0;
    }

    if (m_RCS_OutputUnit == RADAR_CornerReflectorLocation::dBsm)
        return 10.0 * m_safe_log10(sigma);

    return sigma;
}

void RADAR_CornerReflectorLocation_Block::m_compute_simple_xyz_center_and_orientation(
    Vec3& center, Mat3& bodyToGlobal, double rollDeg, double pitchDeg, double yawDeg)
{
    const double t = static_cast<double>(m_sampleIndex) * m_TimeStep;
    const double t2 = t * t;
    const double t3 = t2 * t;

    const int sPos = static_cast<int>(m_Position_Initial_XYZData.size());
    const double x0 = m_get_array_value(m_Position_Initial_XYZData, sPos, 0, 0.0);
    const double y0 = m_get_array_value(m_Position_Initial_XYZData, sPos, 1, 0.0);
    const double z0 = m_get_array_value(m_Position_Initial_XYZData, sPos, 2, 0.0);

    if (m_Motion_Mode == RADAR_CornerReflectorLocation::Moving_Mode) {
        const int sVel = static_cast<int>(m_Velocity_Initial_XYZData.size());
        const double vx = m_get_array_value(m_Velocity_Initial_XYZData, sVel, 0, 0.0);
        const double vy = m_get_array_value(m_Velocity_Initial_XYZData, sVel, 1, 0.0);
        const double vz = m_get_array_value(m_Velocity_Initial_XYZData, sVel, 2, 0.0);

        const int sAcc = static_cast<int>(m_Accelerate_XYZData.size());
        const double ax = m_get_array_value(m_Accelerate_XYZData, sAcc, 0, 0.0);
        const double ay = m_get_array_value(m_Accelerate_XYZData, sAcc, 1, 0.0);
        const double az = m_get_array_value(m_Accelerate_XYZData, sAcc, 2, 0.0);

        const int sJerk = static_cast<int>(m_Jerk_XYZData.size());
        const double jx = m_get_array_value(m_Jerk_XYZData, sJerk, 0, 0.0);
        const double jy = m_get_array_value(m_Jerk_XYZData, sJerk, 1, 0.0);
        const double jz = m_get_array_value(m_Jerk_XYZData, sJerk, 2, 0.0);

        center = m_make_vec(
            x0 + vx * t + 0.5 * ax * t2 + jx * t3 / 6.0,
            y0 + vy * t + 0.5 * ay * t2 + jy * t3 / 6.0,
            z0 + vz * t + 0.5 * az * t2 + jz * t3 / 6.0);
    } else {
        center = m_make_vec(x0, y0, z0);
    }

    bodyToGlobal = m_rpy_matrix(rollDeg * kDegToRad, pitchDeg * kDegToRad, yawDeg * kDegToRad);
}

void RADAR_CornerReflectorLocation_Block::m_compute_eci_center_and_orientation(
    Vec3& center, Mat3& bodyToGlobal, double rollDeg, double pitchDeg, double yawDeg)
{
    const double roll = rollDeg * kDegToRad;
    const double pitch = pitchDeg * kDegToRad;
    const double yaw = yawDeg * kDegToRad;

    const double dirEast  = std::cos(pitch) * std::sin(yaw);
    const double dirNorth = std::cos(pitch) * std::cos(yaw);
    const double dirUp    = std::sin(pitch);

    Vec3 dirEcef = m_add(
        m_add(m_scale(m_eastEcef, dirEast), m_scale(m_northEcef, dirNorth)),
        m_scale(m_upEcef, dirUp));
    dirEcef = m_normalize(dirEcef, m_northEcef);

    const double t = static_cast<double>(m_sampleIndex) * m_TimeStep;
    double distance = 0.0;
    if (m_Motion_Mode == RADAR_CornerReflectorLocation::Moving_Mode)
        distance = m_Velocity_Initial * t + 0.5 * m_Accelerate_Initial * t * t;

    const Vec3 centerEcef = m_add(m_p0Ecef, m_scale(dirEcef, distance));
    const double theta = kEarthRotRate * t;
    const Mat3 ecefToEci = m_ecef_to_eci_matrix(theta);
    center = m_mat_vec(ecefToEci, centerEcef);

    const Mat3 bodyToNed = m_rpy_matrix(roll, pitch, yaw);
    const Mat3 nedToEcef = m_ned_to_ecef_matrix(m_northEcef, m_eastEcef, m_upEcef);
    bodyToGlobal = m_mat_mul(ecefToEci, m_mat_mul(nedToEcef, bodyToNed));
}

// ============================================================================
// Setup
// ============================================================================

bool RADAR_CornerReflectorLocation_Block::Setup()
{
    Block::Setup();

    m_inputBuffer.clear();
    while (!m_outputQueue.empty()) m_outputQueue.pop();

    if (m_NumberOfCornerReflector <= 0) m_NumberOfCornerReflector = 1;
    m_sampleIndex   = 0;
    m_userPathIndex = 0;

    if (!m_validate_params()) return false;

    const int sPosInit = static_cast<int>(m_Position_InitialData.size());
    const double lonDeg = m_get_array_value(m_Position_InitialData, sPosInit, 0, 0.0);
    const double latDeg = m_get_array_value(m_Position_InitialData, sPosInit, 1, 0.0);
    const double h      = m_get_array_value(m_Position_InitialData, sPosInit, 2, 0.0);

    m_lonRad = lonDeg * kDegToRad;
    m_latRad = latDeg * kDegToRad;

    m_p0Ecef = m_lla_to_ecef(m_lonRad, m_latRad, h);

    const double sinLon = std::sin(m_lonRad), cosLon = std::cos(m_lonRad);
    const double sinLat = std::sin(m_latRad), cosLat = std::cos(m_latRad);
    m_eastEcef  = m_make_vec(-sinLon, cosLon, 0.0);
    m_northEcef = m_make_vec(-sinLat * cosLon, -sinLat * sinLon, cosLat);
    m_upEcef    = m_make_vec(cosLat * cosLon, cosLat * sinLon, sinLat);

    m_load_user_file();

    return true;
}

// ============================================================================
// Run — 双模式分发
// ============================================================================

bool RADAR_CornerReflectorLocation_Block::Run()
{
    if (IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

// ============================================================================
// DataStreamRun — 数据流模式，全内联算法
// ============================================================================

bool RADAR_CornerReflectorLocation_Block::DataStreamRun()
{
    // ① 读取输入
    double roll = 0, pitch = 0, yaw = 0;
    double rx = 0, ry = 0, rz = 0;
    double freqIn = m_CarrierFreq;

    {
        auto d = ReadInputData<double>(GetInputPortName(0));
        if (!d.empty()) roll = d[0];
    }
    {
        auto d = ReadInputData<double>(GetInputPortName(1));
        if (!d.empty()) pitch = d[0];
    }
    {
        auto d = ReadInputData<double>(GetInputPortName(2));
        if (!d.empty()) yaw = d[0];
    }
    {
        auto d = ReadInputData<double>(GetInputPortName(3));
        if (!d.empty()) rx = d[0];
    }
    {
        auto d = ReadInputData<double>(GetInputPortName(4));
        if (!d.empty()) ry = d[0];
    }
    {
        auto d = ReadInputData<double>(GetInputPortName(5));
        if (!d.empty()) rz = d[0];
    }
    {
        auto d = ReadInputData<double>(GetInputPortName(6));
        if (!d.empty()) freqIn = d[0];
    }

    const int n = (m_NumberOfCornerReflector > 0) ? m_NumberOfCornerReflector : 1;

    // User_Defined
    if (m_Trajectory_Mode == RADAR_CornerReflectorLocation::User_Defined) {
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

    // ECI / SimpleXYZ
    Vec3 center = m_make_vec(0.0, 0.0, 0.0);
    Mat3 bodyToGlobal = m_identity_mat();

    if (m_Trajectory_Mode == RADAR_CornerReflectorLocation::SimpleXYZ_Frame)
        m_compute_simple_xyz_center_and_orientation(center, bodyToGlobal, roll, pitch, yaw);
    else
        m_compute_eci_center_and_orientation(center, bodyToGlobal, roll, pitch, yaw);

    const double freqHz = m_get_carrier_freq(freqIn);

    std::vector<Vec3> positions;
    std::vector<double> rcsValues;
    positions.reserve(static_cast<std::size_t>(n));
    rcsValues.reserve(static_cast<std::size_t>(n));

    for (int i = 0; i < n; ++i) {
        const Vec3 locBody = m_get_corner_loc(i);

        const double rollOff  = m_get_array_broadcast(m_CornerRollOffsetData,  static_cast<int>(m_CornerRollOffsetData.size()),  i, 0.0) * kDegToRad;
        const double pitchOff = m_get_array_broadcast(m_CornerPitchOffsetData, static_cast<int>(m_CornerPitchOffsetData.size()), i, 0.0) * kDegToRad;
        const double yawOff   = m_get_array_broadcast(m_CornerYawOffsetData,   static_cast<int>(m_CornerYawOffsetData.size()),   i, 0.0) * kDegToRad;

        const Mat3 cornerOffsetRot = m_rpy_matrix(rollOff, pitchOff, yawOff);
        const Mat3 cornerBodyToGlobal = m_mat_mul(bodyToGlobal, cornerOffsetRot);

        Vec3 reflectorPos = m_add(center, m_mat_vec(bodyToGlobal, locBody));
        const Vec3 phaseOffsetBody = m_get_phase_center_offset(i);
        reflectorPos = m_add(reflectorPos, m_mat_vec(cornerBodyToGlobal, phaseOffsetBody));

        const double sigma = m_calc_corner_rcs(i, reflectorPos, cornerBodyToGlobal, freqHz, rx, ry, rz);
        positions.push_back(reflectorPos);
        rcsValues.push_back(sigma);
    }

    std::vector<SystemVueModelBuilder::DoubleMatrix> posOut(static_cast<std::size_t>(n));
    std::vector<double> rcsOut(static_cast<std::size_t>(n));
    for (int i = 0; i < n; ++i) {
        SystemVueModelBuilder::DoubleMatrix pm(3, 1);
        pm(0, 0) = positions[static_cast<std::size_t>(i)].x;
        pm(1, 0) = positions[static_cast<std::size_t>(i)].y;
        pm(2, 0) = positions[static_cast<std::size_t>(i)].z;
        posOut[i] = pm;
        rcsOut[i] = rcsValues[static_cast<std::size_t>(i)];
    }

    WriteOutputData(GetOutputPortName(0), posOut);
    WriteOutputData(GetOutputPortName(1), rcsOut);

    ++m_sampleIndex;
    return true;
}

// ============================================================================
// TimeDrivenRun — 变步长模式：输入存 buffer，处理后入队，队列非空则输出
// ============================================================================

bool RADAR_CornerReflectorLocation_Block::TimeDrivenRun()
{
    // ① 累积输入
    {
        auto d0 = ReadInputData<double>(GetInputPortName(0));
        auto d1 = ReadInputData<double>(GetInputPortName(1));
        auto d2 = ReadInputData<double>(GetInputPortName(2));
        auto d3 = ReadInputData<double>(GetInputPortName(3));
        auto d4 = ReadInputData<double>(GetInputPortName(4));
        auto d5 = ReadInputData<double>(GetInputPortName(5));
        auto d6 = ReadInputData<double>(GetInputPortName(6));

        if (!d0.empty() || !d1.empty() || !d2.empty() || !d3.empty() ||
            !d4.empty() || !d5.empty() || !d6.empty()) {
            InputSnapshot in;
            in.roll   = d0.empty() ? 0.0 : d0[0];
            in.pitch  = d1.empty() ? 0.0 : d1[0];
            in.yaw    = d2.empty() ? 0.0 : d2[0];
            in.radarX = d3.empty() ? 0.0 : d3[0];
            in.radarY = d4.empty() ? 0.0 : d4[0];
            in.radarZ = d5.empty() ? 0.0 : d5[0];
            in.carrierFreqIn = d6.empty() ? m_CarrierFreq : d6[0];
            m_inputBuffer.push_back(in);
        }
    }

    // ② 处理所有累积输入 → 入队
    while (!m_inputBuffer.empty()) {
        InputSnapshot in = m_inputBuffer.front();
        m_inputBuffer.erase(m_inputBuffer.begin());

        const int n = (m_NumberOfCornerReflector > 0) ? m_NumberOfCornerReflector : 1;

        OutputFrame out;
        out.pos.resize(static_cast<std::size_t>(n));
        out.rcs.resize(static_cast<std::size_t>(n));

        // User_Defined
        if (m_Trajectory_Mode == RADAR_CornerReflectorLocation::User_Defined) {
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

        // ECI / SimpleXYZ
        Vec3 center = m_make_vec(0.0, 0.0, 0.0);
        Mat3 bodyToGlobal = m_identity_mat();

        if (m_Trajectory_Mode == RADAR_CornerReflectorLocation::SimpleXYZ_Frame)
            m_compute_simple_xyz_center_and_orientation(center, bodyToGlobal, in.roll, in.pitch, in.yaw);
        else
            m_compute_eci_center_and_orientation(center, bodyToGlobal, in.roll, in.pitch, in.yaw);

        const double freqHz = m_get_carrier_freq(in.carrierFreqIn);

        for (int i = 0; i < n; ++i) {
            const Vec3 locBody = m_get_corner_loc(i);

            const double rollOff  = m_get_array_broadcast(m_CornerRollOffsetData,  static_cast<int>(m_CornerRollOffsetData.size()),  i, 0.0) * kDegToRad;
            const double pitchOff = m_get_array_broadcast(m_CornerPitchOffsetData, static_cast<int>(m_CornerPitchOffsetData.size()), i, 0.0) * kDegToRad;
            const double yawOff   = m_get_array_broadcast(m_CornerYawOffsetData,   static_cast<int>(m_CornerYawOffsetData.size()),   i, 0.0) * kDegToRad;

            const Mat3 cornerOffsetRot = m_rpy_matrix(rollOff, pitchOff, yawOff);
            const Mat3 cornerBodyToGlobal = m_mat_mul(bodyToGlobal, cornerOffsetRot);

            Vec3 reflectorPos = m_add(center, m_mat_vec(bodyToGlobal, locBody));
            const Vec3 phaseOffsetBody = m_get_phase_center_offset(i);
            reflectorPos = m_add(reflectorPos, m_mat_vec(cornerBodyToGlobal, phaseOffsetBody));

            const double sigma = m_calc_corner_rcs(i, reflectorPos, cornerBodyToGlobal, freqHz, in.radarX, in.radarY, in.radarZ);

            SystemVueModelBuilder::DoubleMatrix pm(3, 1);
            pm(0, 0) = reflectorPos.x; pm(1, 0) = reflectorPos.y; pm(2, 0) = reflectorPos.z;
            out.pos[i] = pm;
            out.rcs[i] = sigma;
        }

        ++m_sampleIndex;
        m_outputQueue.push(out);
    }

    // ③ 出队写入：outputQueue 不为空就输出一次
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

bool RADAR_CornerReflectorLocation_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_algo = std::make_unique<RADAR_CornerReflectorLocation>();

    SetDefaultParameters();

    try { m_Trajectory_Mode = ConvertStringToTrajectoryMode(getParameter("Trajectory_Mode").Value); } catch (...) {}
    try { m_Motion_Mode     = ConvertStringToMotionMode(getParameter("Motion_Mode").Value);         } catch (...) {}
    try { m_NumberOfCornerReflector = std::stoi(getParameter("NumberOfCornerReflector").Value);     } catch (...) {}
    try { m_ReflectorType   = ConvertStringToReflectorType(getParameter("ReflectorType").Value);    } catch (...) {}
    try { m_RCS_Model       = ConvertStringToRCSModel(getParameter("RCS_Model").Value);             } catch (...) {}
    try { m_RCS_OutputUnit  = ConvertStringToRCSOutputUnit(getParameter("RCS_OutputUnit").Value);   } catch (...) {}
    try { m_FileName        = getParameter("FileName").Value; } catch (...) {}

    try { m_CornerLocData            = ParseStringToDoubleVector(getParameter("CornerLoc").Value);            } catch (...) {}
    try { m_EdgeLengthData           = ParseStringToDoubleVector(getParameter("EdgeLength").Value);           } catch (...) {}
    try { m_EfficiencyData           = ParseStringToDoubleVector(getParameter("Efficiency").Value);           } catch (...) {}
    try { m_CornerRollOffsetData     = ParseStringToDoubleVector(getParameter("CornerRollOffset").Value);     } catch (...) {}
    try { m_CornerPitchOffsetData    = ParseStringToDoubleVector(getParameter("CornerPitchOffset").Value);    } catch (...) {}
    try { m_CornerYawOffsetData      = ParseStringToDoubleVector(getParameter("CornerYawOffset").Value);      } catch (...) {}
    try { m_PhaseCenterOffsetData    = ParseStringToDoubleVector(getParameter("PhaseCenterOffset").Value);    } catch (...) {}
    try { m_Radar_Position_XYZData   = ParseStringToDoubleVector(getParameter("Radar_Position_XYZ").Value);   } catch (...) {}
    try { m_Position_InitialData     = ParseStringToDoubleVector(getParameter("Position_Initial").Value);     } catch (...) {}
    try { m_Position_Initial_XYZData = ParseStringToDoubleVector(getParameter("Position_Initial_XYZ").Value); } catch (...) {}
    try { m_Velocity_Initial_XYZData = ParseStringToDoubleVector(getParameter("Velocity_Initial_XYZ").Value); } catch (...) {}
    try { m_Accelerate_XYZData       = ParseStringToDoubleVector(getParameter("Accelerate_XYZ").Value);       } catch (...) {}
    try { m_Jerk_XYZData             = ParseStringToDoubleVector(getParameter("Jerk_XYZ").Value);             } catch (...) {}

    try { m_CarrierFreq        = std::stod(getParameter("CarrierFreq").Value);         } catch (...) {}
    try { m_BoresightHalfAngle = std::stod(getParameter("BoresightHalfAngle").Value);  } catch (...) {}
    try { m_RCS_Floor          = std::stod(getParameter("RCS_Floor").Value);           } catch (...) {}
    try { m_Velocity_Initial   = std::stod(getParameter("Velocity_Initial").Value);    } catch (...) {}
    try { m_Accelerate_Initial = std::stod(getParameter("Accelerate_Initial").Value);  } catch (...) {}
    try { m_TimeStep           = std::stod(getParameter("TimeStep").Value);            } catch (...) {}

    SetAlgoParameters();

    if (!m_algo->Setup()) return false;

    // 输入端口
    AddInputPort("Roll",          m_algo->Roll,          1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddInputPort("Pitch",         m_algo->Pitch,         1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddInputPort("Yaw",           m_algo->Yaw,           1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddInputPort("RadarX",        m_algo->RadarX,        1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddInputPort("RadarY",        m_algo->RadarY,        1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddInputPort("RadarZ",        m_algo->RadarZ,        1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddInputPort("CarrierFreqIn", m_algo->CarrierFreqIn, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);

    // 输出端口
    AddOutputPort("Pos",       m_algo->Pos,       m_NumberOfCornerReflector, Block::DataType::MATRIX_DOUBLE_BUS);
    AddOutputPort("CornerRCS", m_algo->CornerRCS, m_NumberOfCornerReflector, Block::DataType::DOUBLE_BUS);

    return true;
}
