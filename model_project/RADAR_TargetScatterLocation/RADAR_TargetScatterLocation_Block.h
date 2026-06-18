#ifndef RADAR_TARGETSCATTERLOCATION_BLOCK_H
#define RADAR_TARGETSCATTERLOCATION_BLOCK_H

#include "Block.h"
#include "RADAR_TargetScatterLocation.h"

#include <memory>
#include <queue>
#include <string>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_TargetScatterLocation_Block : public SystemVueModelBuilder::Block
{
public:
    RADAR_TargetScatterLocation_Block(const std::string& name);
    ~RADAR_TargetScatterLocation_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

private:
    void SetDefaultParameters();
    void SetAlgoParameters();

    bool DataStreamRun();
    bool TimeDrivenRun();

    // ===== 字符串转换 =====
    RADAR_TargetScatterLocation::Trajectory_ModeEnum ConvertStringToTrajectoryMode(const std::string& value);
    RADAR_TargetScatterLocation::IsRandomErrorEnum   ConvertStringToIsRandomError(const std::string& value);
    RADAR_TargetScatterLocation::IsRCSRandomEnum     ConvertStringToIsRCSRandom(const std::string& value);

    // ===== 算法对象（用于端口注册和 Setup） =====
    std::unique_ptr<RADAR_TargetScatterLocation> m_algo;

    // ===== 枚举参数 =====
    RADAR_TargetScatterLocation::Trajectory_ModeEnum m_Trajectory_Mode;
    RADAR_TargetScatterLocation::IsRandomErrorEnum   m_IsRandomError;
    RADAR_TargetScatterLocation::IsRCSRandomEnum     m_IsRCSRandom;

    // ===== 标量参数 =====
    int    m_NumberOfTargetScatter;
    double m_Velocity_Initial;
    double m_Accelerate_Initial;
    double m_Accelerate_Variance;
    double m_TimeStep;

    // ===== UserDefined 参数 =====
    std::string m_FileName;

    // ===== 数组参数（使用 vector<double>） =====
    std::vector<double> m_ScatterLocData;
    std::vector<double> m_RCSData;
    std::vector<double> m_RCS_VarianceData;
    std::vector<double> m_DurationTimeData;
    std::vector<double> m_Position_InitialData;
    std::vector<double> m_Position_Initial_XYZData;
    std::vector<double> m_Velocity_Initial_XYZData;
    std::vector<double> m_Accelerate_XYZData;
    std::vector<double> m_Jerk_XYZData;

    // ===== 3D 向量 =====
    struct Vec3 { double x; double y; double z; };

    struct UserSample
    {
        std::vector<Vec3> pos;
        std::vector<double> rcs;
    };

    // ===== 算法内部状态 =====
    unsigned long long m_sampleIndex;
    Vec3               m_p0Ecef;
    Vec3               m_eastEcef;
    Vec3               m_northEcef;
    Vec3               m_upEcef;
    Vec3               m_motionAccumEcef;
    double             m_lonRad;
    double             m_latRad;
    std::vector<UserSample> m_userPath;
    std::size_t             m_userPathIndex;
    UserSample              m_lastUserSample;

    // ===== 变步长缓冲 =====
    struct InputSnapshot {
        double roll, pitch, yaw;
    };
    std::vector<InputSnapshot> m_inputBuffer;

    struct OutputFrame {
        std::vector<SystemVueModelBuilder::DoubleMatrix> pos;
        std::vector<double> rcs;
    };
    std::queue<OutputFrame> m_outputQueue;

    // ===== 基本工具函数 =====
    static double m_get_array_value(const std::vector<double>& data, int size, int idx, double defval);
    static Vec3   m_make_vec(double x, double y, double z);
    static Vec3   m_add(const Vec3& a, const Vec3& b);
    static Vec3   m_scale(const Vec3& a, double s);
    static double m_dot(const Vec3& a, const Vec3& b);
    static Vec3   m_lla_to_ecef(double lonRad, double latRad, double h);
    static Vec3   m_rotate_z(const Vec3& v, double theta);

    // ===== 算法辅助函数 =====
    bool validateParams() const;
    bool loadUserFile();
    void initEmptyUserSample();

    Vec3   getScatterLoc(int idx) const;
    double getRcsBase(int idx) const;
    double calcScatterRcs(int idx, unsigned long long k) const;
    double baseRandomPosition(unsigned long long k) const;
    double baseRandomRcs(unsigned long long blockIndex, int scatterIndex) const;
};

RegAlgo(RADAR_TargetScatterLocation_Block);

#endif // RADAR_TARGETSCATTERLOCATION_BLOCK_H
