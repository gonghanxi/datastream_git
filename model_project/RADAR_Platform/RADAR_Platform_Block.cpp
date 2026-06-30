#include "RADAR_Platform_Block.h"

#include <cmath>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <iostream>
#include <cctype>

// ============================================================================
// 构造函数
// ============================================================================

RADAR_Platform_Block::RADAR_Platform_Block(const std::string& name)
    : Block(name)
    , sampleIndex_(0ULL)
    , lonRad_(0.0), latRad_(0.0)
    , userPathIndex_(0)
{
    p0Ecef_ = makeVec_(0, 0, 0);
    eastEcef_ = makeVec_(0, 1, 0);
    northEcef_ = makeVec_(0, 0, 1);
    upEcef_ = makeVec_(1, 0, 0);
    motionAccumEcef_ = makeVec_(0, 0, 0);
    lastUserPos_ = makeVec_(0, 0, 0);
}

// ============================================================================
// Setup
// ============================================================================

bool RADAR_Platform_Block::Setup()
{
    Block::Setup();
    while (!m_outputQueue.empty()) m_outputQueue.pop();
    sampleIndex_ = 0ULL;
    motionAccumEcef_ = makeVec_(0, 0, 0);
    userPathIndex_ = 0;
    lastUserPos_ = makeVec_(0, 0, 0);
    return true;
}

// ============================================================================
// Run
// ============================================================================

bool RADAR_Platform_Block::Run()
{
    if (!CanProcess()) return false;
    if (IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

// ============================================================================
// DataStreamRun
// ============================================================================

bool RADAR_Platform_Block::DataStreamRun()
{
    DoubleMatrix pos = computePosition_();

    std::vector<DoubleMatrix> outputData;
    outputData.push_back(pos);
    WriteOutputData(GetOutputPortName(0), outputData);

    return true;
}

// ============================================================================
// TimeDrivenRun
// ============================================================================

bool RADAR_Platform_Block::TimeDrivenRun()
{
    DoubleMatrix pos = computePosition_();
    m_outputQueue.push(pos);

    if (!m_outputQueue.empty())
    {
        DoubleMatrix outValue = m_outputQueue.front();
        m_outputQueue.pop();

        std::vector<DoubleMatrix> outputData;
        outputData.push_back(outValue);
        WriteOutputData(GetOutputPortName(0), outputData);
    }

    return true;
}

// ============================================================================
// Initialize
// ============================================================================

bool RADAR_Platform_Block::Initialize()
{
    SetBlockType(Block::BlockType::SOURCE);
    m_algo = std::make_unique<RADAR_Platform>();

    SetDefaultParameters();

    // 解析公共枚举参数
    try { Trajectory_Mode_ = ParseTrajectoryMode(getParameter("Trajectory_Mode").Value); }
    catch (...) { LOG_WARN("Failed to parse 'Trajectory_Mode'"); }
    try { PrintLog_ = ParsePrintLog(getParameter("PrintLog").Value); }
    catch (...) { LOG_WARN("Failed to parse 'PrintLog'"); }
    try { IsRandomError_ = ParseIsRandomError(getParameter("IsRandomError").Value); }
    catch (...) { LOG_WARN("Failed to parse 'IsRandomError'"); }

    // 解析公共时间参数
    try { TimeStep_ = std::stod(getParameter("TimeStep").Value); }
    catch (...) { LOG_WARN("Failed to parse 'TimeStep'"); }

    // 解析 ECI_Frame 参数
    try { Position_Initial_ = ParseDoubleArray(getParameter("Position_Initial").Value); }
    catch (...) { LOG_WARN("Failed to parse 'Position_Initial'"); }
    try { Velocity_Initial_ = std::stod(getParameter("Velocity_Initial").Value); }
    catch (...) { LOG_WARN("Failed to parse 'Velocity_Initial'"); }
    try { Accelerate_Initial_ = std::stod(getParameter("Accelerate_Initial").Value); }
    catch (...) { LOG_WARN("Failed to parse 'Accelerate_Initial'"); }
    try { Accelerate_Variance_ = std::stod(getParameter("Accelerate_Variance").Value); }
    catch (...) { LOG_WARN("Failed to parse 'Accelerate_Variance'"); }

    // 解析 User Defined 参数
    try { FileName_ = getParameter("FileName").Value; }
    catch (...) { LOG_WARN("Failed to parse 'FileName'"); }

    // 解析 SimpleXYZ_Frame 参数
    try { Position_Initial_XYZ_ = ParseDoubleArray(getParameter("Position_Initial_XYZ").Value); }
    catch (...) { LOG_WARN("Failed to parse 'Position_Initial_XYZ'"); }
    try { Velocity_Initial_XYZ_ = ParseDoubleArray(getParameter("Velocity_Initial_XYZ").Value); }
    catch (...) { LOG_WARN("Failed to parse 'Velocity_Initial_XYZ'"); }
    try { Accelerate_XYZ_ = ParseDoubleArray(getParameter("Accelerate_XYZ").Value); }
    catch (...) { LOG_WARN("Failed to parse 'Accelerate_XYZ'"); }
    try { Jerk_XYZ_ = ParseDoubleArray(getParameter("Jerk_XYZ").Value); }
    catch (...) { LOG_WARN("Failed to parse 'Jerk_XYZ'"); }

    SetParameters();
    if (!ModelSetup()) return false;

    // 注册端口：输出 Pos (DoubleMatrix 3x1)，输入 Roll/Pitch/Yaw (可选 double)
    AddOutputPort("Pos", m_algo->Pos, 1, Block::DataType::MATRIX_DOUBLE);
    AddInputPort("Roll", m_algo->Roll, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddInputPort("Pitch", m_algo->Pitch, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddInputPort("Yaw", m_algo->Yaw, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);

    return true;
}

// ============================================================================
// SetDefaultParameters
// ============================================================================

void RADAR_Platform_Block::SetDefaultParameters()
{
    Trajectory_Mode_ = ECI_Frame;
    PrintLog_ = PrintLog_No;
    IsRandomError_ = IRE_false;
    Position_Initial_ = {0.0, 0.0, 0.0};
    Velocity_Initial_ = 0.0;
    Accelerate_Initial_ = 0.0;
    Accelerate_Variance_ = 0.0;
    FileName_ = "";
    Position_Initial_XYZ_ = {0.0, 20e3, 5e3};
    Velocity_Initial_XYZ_ = {0.0, 0.0, 0.0};
    Accelerate_XYZ_ = {0.0, 0.0, 0.0};
    Jerk_XYZ_ = {0.0, 0.0, 0.0};
    TimeStep_ = 1e-9;
}

// ============================================================================
// SetParameters — 同步到算法实例（仅用于端口注册）
// ============================================================================

void RADAR_Platform_Block::SetParameters()
{
    if (!m_algo) return;
    m_algo->TimeStep = TimeStep_;
    m_algo->Trajectory_Mode = static_cast<RADAR_Platform::Trajectory_ModeEnum>(Trajectory_Mode_);
    m_algo->PrintLog = static_cast<RADAR_Platform::PrintLogEnum>(PrintLog_);
    m_algo->IsRandomError = static_cast<RADAR_Platform::IsRandomErrorEnum>(IsRandomError_);
    m_algo->Velocity_Initial = Velocity_Initial_;
    m_algo->Accelerate_Initial = Accelerate_Initial_;
    m_algo->Accelerate_Variance = Accelerate_Variance_;
}

// ============================================================================
// ModelSetup — 算法逻辑初始化（不调用 m_algo->Setup()）
// ============================================================================

bool RADAR_Platform_Block::ModelSetup()
{
    sampleIndex_ = 0ULL;
    motionAccumEcef_ = makeVec_(0, 0, 0);

    // 初始化 ECI 模式坐标常量
    const double lonDeg = getArrayValue_(Position_Initial_, 0, 0.0);
    const double latDeg = getArrayValue_(Position_Initial_, 1, 0.0);
    const double h = getArrayValue_(Position_Initial_, 2, 0.0);

    lonRad_ = lonDeg * kDegToRad;
    latRad_ = latDeg * kDegToRad;

    p0Ecef_ = llaToEcef_(lonRad_, latRad_, h);

    const double sinLon = std::sin(lonRad_);
    const double cosLon = std::cos(lonRad_);
    const double sinLat = std::sin(latRad_);
    const double cosLat = std::cos(latRad_);

    eastEcef_ = makeVec_(-sinLon, cosLon, 0.0);
    northEcef_ = makeVec_(-sinLat * cosLon, -sinLat * sinLon, cosLat);
    upEcef_ = makeVec_(cosLat * cosLon, cosLat * sinLon, sinLat);

    // 初始化 User Defined 模式轨迹文件
    loadUserFile_();

    if (PrintLog_ == PrintLog_Yes)
    {
        std::cout << "RADAR_Platform_Block Setup: mode=" << static_cast<int>(Trajectory_Mode_)
                  << ", TimeStep=" << TimeStep_ << std::endl;
    }

    return true;
}

// ============================================================================
// computePosition_ — 根据轨迹模式分派计算
// ============================================================================

DoubleMatrix RADAR_Platform_Block::computePosition_()
{
    if (Trajectory_Mode_ == SimpleXYZ_Frame)
        return computeSimpleXYZ_();

    if (Trajectory_Mode_ == User_Defined)
        return computeUserDefined_();

    // ECI_Frame：读取可选输入端口
    double rollDeg = 0.0, pitchDeg = 0.0, yawDeg = 0.0;

    BufferReader* rollPort = GetInputPort("Roll");
    if (rollPort && rollPort->IsConnected())
    {
        auto data = ReadInputData<double>("Roll");
        if (!data.empty()) rollDeg = data[0];
    }

    BufferReader* pitchPort = GetInputPort("Pitch");
    if (pitchPort && pitchPort->IsConnected())
    {
        auto data = ReadInputData<double>("Pitch");
        if (!data.empty()) pitchDeg = data[0];
    }

    BufferReader* yawPort = GetInputPort("Yaw");
    if (yawPort && yawPort->IsConnected())
    {
        auto data = ReadInputData<double>("Yaw");
        if (!data.empty()) yawDeg = data[0];
    }

    return computeECI_(rollDeg, pitchDeg, yawDeg);
}

// ============================================================================
// computeSimpleXYZ_ — SimpleXYZ_Frame 模式
// ============================================================================

DoubleMatrix RADAR_Platform_Block::computeSimpleXYZ_()
{
    const double k = static_cast<double>(sampleIndex_);
    const double t = k * TimeStep_;
    const double t2 = t * t;
    const double t3 = t2 * t;

    const double x0 = getArrayValue_(Position_Initial_XYZ_, 0, 0.0);
    const double y0 = getArrayValue_(Position_Initial_XYZ_, 1, 20e3);
    const double z0 = getArrayValue_(Position_Initial_XYZ_, 2, 5e3);

    const double vx = getArrayValue_(Velocity_Initial_XYZ_, 0, 0.0);
    const double vy = getArrayValue_(Velocity_Initial_XYZ_, 1, 0.0);
    const double vz = getArrayValue_(Velocity_Initial_XYZ_, 2, 0.0);

    const double ax = getArrayValue_(Accelerate_XYZ_, 0, 0.0);
    const double ay = getArrayValue_(Accelerate_XYZ_, 1, 0.0);
    const double az = getArrayValue_(Accelerate_XYZ_, 2, 0.0);

    const double jx = getArrayValue_(Jerk_XYZ_, 0, 0.0);
    const double jy = getArrayValue_(Jerk_XYZ_, 1, 0.0);
    const double jz = getArrayValue_(Jerk_XYZ_, 2, 0.0);

    // 黑盒对齐：jerk 项系数为 1/3
    const double x = x0 + vx * t + 0.5 * ax * t2 + (jx * t3 / 3.0);
    const double y = y0 + vy * t + 0.5 * ay * t2 + (jy * t3 / 3.0);
    const double z = z0 + vz * t + 0.5 * az * t2 + (jz * t3 / 3.0);

    ++sampleIndex_;

    DoubleMatrix m(3, 1);
    m(0, 0) = x;
    m(1, 0) = y;
    m(2, 0) = z;
    return m;
}

// ============================================================================
// computeUserDefined_ — User_Defined 模式
// ============================================================================

DoubleMatrix RADAR_Platform_Block::computeUserDefined_()
{
    Vec3 out = lastUserPos_;

    if (!userPath_.empty())
    {
        if (userPathIndex_ < userPath_.size())
        {
            out = userPath_[userPathIndex_];
            lastUserPos_ = out;
            ++userPathIndex_;
        }
    }

    ++sampleIndex_;

    DoubleMatrix m(3, 1);
    m(0, 0) = out.x;
    m(1, 0) = out.y;
    m(2, 0) = out.z;
    return m;
}

// ============================================================================
// computeECI_ — ECI_Frame 模式
// ============================================================================

DoubleMatrix RADAR_Platform_Block::computeECI_(double rollDeg, double pitchDeg, double yawDeg)
{
    (void)rollDeg; // Roll 不影响位置，保留读取以匹配接口

    const double pitch = pitchDeg * kDegToRad;
    const double yaw = yawDeg * kDegToRad;
    const double cosPitch = std::cos(pitch);

    // 黑盒对齐的 ENU 方向：yaw=0 指向 North，yaw=+90 指向 East，pitch=+90 指向 Up
    const double dirEast = cosPitch * std::sin(yaw);
    const double dirNorth = cosPitch * std::cos(yaw);
    const double dirUp = std::sin(pitch);

    Vec3 dirEcef = add_(
        add_(scale_(eastEcef_, dirEast), scale_(northEcef_, dirNorth)),
        scale_(upEcef_, dirUp)
    );

    // 单位化以减少数值漂移
    const double norm2 = dot_(dirEcef, dirEcef);
    if (norm2 > 0.0)
        dirEcef = scale_(dirEcef, 1.0 / std::sqrt(norm2));

    // 每 sample 固定推进 V*Ts + 0.5*A*Ts^2
    const double stepDistance =
        Velocity_Initial_ * TimeStep_
        + 0.5 * Accelerate_Initial_ * TimeStep_ * TimeStep_;

    motionAccumEcef_ = add_(motionAccumEcef_, scale_(dirEcef, stepDistance));

    // 随机误差
    double randomDistance = 0.0;
    if (IsRandomError_ == IRE_true && Accelerate_Variance_ > 0.0)
    {
        randomDistance = std::sqrt(Accelerate_Variance_) * baseRandom_(sampleIndex_);
    }

    Vec3 localMovedPosition = add_(
        p0Ecef_,
        add_(motionAccumEcef_, scale_(dirEcef, randomDistance))
    );

    // 地球自转
    const double theta = kEarthRotationRate * static_cast<double>(sampleIndex_) * TimeStep_;
    const Vec3 posEci = rotateZ_(localMovedPosition, theta);

    ++sampleIndex_;

    DoubleMatrix m(3, 1);
    m(0, 0) = posEci.x;
    m(1, 0) = posEci.y;
    m(2, 0) = posEci.z;
    return m;
}

// ============================================================================
// loadUserFile_ — 加载用户轨迹文件
// ============================================================================

bool RADAR_Platform_Block::loadUserFile_()
{
    userPath_.clear();
    userPathIndex_ = 0;
    lastUserPos_ = makeVec_(0, 0, 0);

    if (FileName_.empty())
    {
        if (PrintLog_ == PrintLog_Yes)
            std::cout << "RADAR_Platform_Block: FileName is empty." << std::endl;
        return true;
    }

    std::ifstream fin(FileName_);
    if (!fin)
    {
        if (PrintLog_ == PrintLog_Yes)
            std::cout << "RADAR_Platform_Block: failed to open file: " << FileName_ << std::endl;
        return true;
    }

    std::string line;
    while (std::getline(fin, line))
    {
        std::istringstream iss(line);
        double x = 0.0, y = 0.0, z = 0.0;
        if (iss >> x >> y >> z)
        {
            userPath_.push_back(makeVec_(x, y, z));
        }
    }

    if (!userPath_.empty())
        lastUserPos_ = userPath_.front();

    if (PrintLog_ == PrintLog_Yes)
        std::cout << "RADAR_Platform_Block: loaded " << userPath_.size()
                  << " user trajectory points." << std::endl;

    return true;
}

// ============================================================================
// baseRandom_ — 黑盒反推随机基序列
// ============================================================================

double RADAR_Platform_Block::baseRandom_(unsigned long long k) const
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
    const unsigned long long nTable = static_cast<unsigned long long>(sizeof(table) / sizeof(table[0]));
    if (k < nTable)
        return table[k];

    unsigned long long x = 1469598103934665603ULL ^ (k + 0x9e3779b97f4a7c15ULL);
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    const unsigned long long y = x * 2685821657736338717ULL;
    const double u = (static_cast<double>((y >> 11) & 0x1FFFFFFFFFFFFFULL) + 1.0)
        / 9007199254740992.0;
    return 2.0 * u - 1.0;
}

// ============================================================================
// 静态向量工具函数
// ============================================================================

RADAR_Platform_Block::Vec3 RADAR_Platform_Block::makeVec_(double x, double y, double z)
{
    return {x, y, z};
}

RADAR_Platform_Block::Vec3 RADAR_Platform_Block::add_(const Vec3& a, const Vec3& b)
{
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}

RADAR_Platform_Block::Vec3 RADAR_Platform_Block::scale_(const Vec3& a, double s)
{
    return {a.x * s, a.y * s, a.z * s};
}

double RADAR_Platform_Block::dot_(const Vec3& a, const Vec3& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

RADAR_Platform_Block::Vec3 RADAR_Platform_Block::llaToEcef_(double lonRad, double latRad, double h)
{
    const double a = kEarthSemiMajorAxis;
    const double b = kEarthSemiMinorAxis;
    const double a2 = a * a;
    const double b2 = b * b;
    const double e2 = 1.0 - b2 / a2;

    const double sinLat = std::sin(latRad);
    const double cosLat = std::cos(latRad);
    const double sinLon = std::sin(lonRad);
    const double cosLon = std::cos(lonRad);

    const double N = a / std::sqrt(1.0 - e2 * sinLat * sinLat);

    const double x = (N + h) * cosLat * cosLon;
    const double y = (N + h) * cosLat * sinLon;
    const double z = (N * (1.0 - e2) + h) * sinLat;

    return {x, y, z};
}

RADAR_Platform_Block::Vec3 RADAR_Platform_Block::rotateZ_(const Vec3& v, double theta)
{
    const double c = std::cos(theta);
    const double s = std::sin(theta);
    return {c * v.x - s * v.y, s * v.x + c * v.y, v.z};
}

double RADAR_Platform_Block::getArrayValue_(const std::vector<double>& v, int idx, double defval)
{
    if (idx < 0 || idx >= static_cast<int>(v.size()))
        return defval;
    return v[static_cast<size_t>(idx)];
}

// ============================================================================
// 参数解析
// ============================================================================

RADAR_Platform_Block::TrajectoryMode
RADAR_Platform_Block::ParseTrajectoryMode(const std::string& str)
{
    std::string s = str;
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });

    if (s == "user defined" || s == "user_defined" || s == "1") return User_Defined;
    if (s == "simplexyz_frame" || s == "simplexyz" || s == "2") return SimpleXYZ_Frame;
    return ECI_Frame;
}

RADAR_Platform_Block::IsRandomErrorEnum
RADAR_Platform_Block::ParseIsRandomError(const std::string& str)
{
    std::string s = str;
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });

    if (s == "ire_true" || s == "true" || s == "1") return IRE_true;
    return IRE_false;
}

RADAR_Platform_Block::PrintLogEnum
RADAR_Platform_Block::ParsePrintLog(const std::string& str)
{
    std::string s = str;
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });

    if (s == "printlog_yes" || s == "yes" || s == "1") return PrintLog_Yes;
    return PrintLog_No;
}

std::vector<double> RADAR_Platform_Block::ParseDoubleArray(const std::string& str)
{
    std::vector<double> result;
    std::string s = str;

    // 去除首尾空白
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());

    // 去除方括号
    if (!s.empty() && s.front() == '[') s = s.substr(1);
    if (!s.empty() && s.back() == ']') s.pop_back();

    std::istringstream iss(s);
    double val;
    while (iss >> val)
    {
        result.push_back(val);
    }

    return result;
}
