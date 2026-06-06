#ifndef RADAR_CORNERREFLECTORLOCATION_BLOCK_H
#define RADAR_CORNERREFLECTORLOCATION_BLOCK_H

#include "Block.h"
#include "RADAR_CornerReflectorLocation.h"

#include <memory>
#include <queue>
#include <string>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_CornerReflectorLocation_Block : public SystemVueModelBuilder::Block
{
public:
    RADAR_CornerReflectorLocation_Block(const std::string& name);
    ~RADAR_CornerReflectorLocation_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

private:
    void SetDefaultParameters();

    bool DataStreamRun();
    bool TimeDrivenRun();

    // ===== 字符串转换（5 个枚举） =====
    RADAR_CornerReflectorLocation::Trajectory_ModeEnum  ConvertStringToTrajectoryMode(const std::string& value);
    RADAR_CornerReflectorLocation::Motion_ModeEnum      ConvertStringToMotionMode(const std::string& value);
    RADAR_CornerReflectorLocation::ReflectorTypeEnum    ConvertStringToReflectorType(const std::string& value);
    RADAR_CornerReflectorLocation::RCSModelEnum         ConvertStringToRCSModel(const std::string& value);
    RADAR_CornerReflectorLocation::RCSOutputUnitEnum    ConvertStringToRCSOutputUnit(const std::string& value);

    // ===== 算法对象（仅用于端口注册和 Setup） =====
    std::unique_ptr<RADAR_CornerReflectorLocation> m_algo;

    // ===== 枚举参数 =====
    RADAR_CornerReflectorLocation::Trajectory_ModeEnum m_Trajectory_Mode;
    RADAR_CornerReflectorLocation::Motion_ModeEnum     m_Motion_Mode;
    int                                                m_NumberOfCornerReflector;
    RADAR_CornerReflectorLocation::ReflectorTypeEnum   m_ReflectorType;
    RADAR_CornerReflectorLocation::RCSModelEnum        m_RCS_Model;
    RADAR_CornerReflectorLocation::RCSOutputUnitEnum   m_RCS_OutputUnit;

    // ===== UserDefined 参数 =====
    std::string m_FileName;

    // ===== 指针数组参数（转为 vector<double>） =====
    std::vector<double> m_CornerLocData;
    std::vector<double> m_EdgeLengthData;
    std::vector<double> m_EfficiencyData;
    std::vector<double> m_CornerRollOffsetData;
    std::vector<double> m_CornerPitchOffsetData;
    std::vector<double> m_CornerYawOffsetData;
    std::vector<double> m_PhaseCenterOffsetData;
    std::vector<double> m_Radar_Position_XYZData;
    std::vector<double> m_Position_InitialData;
    std::vector<double> m_Position_Initial_XYZData;
    std::vector<double> m_Velocity_Initial_XYZData;
    std::vector<double> m_Accelerate_XYZData;
    std::vector<double> m_Jerk_XYZData;

    // ===== 标量参数 =====
    double m_CarrierFreq;
    double m_BoresightHalfAngle;
    double m_RCS_Floor;
    double m_Velocity_Initial;
    double m_Accelerate_Initial;
    double m_TimeStep;

    // ===== 3D 向量 & 3x3 矩阵 =====
    struct Vec3 { double x; double y; double z; };
    struct Mat3 { double m[3][3]; };

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
    double             m_lonRad;
    double             m_latRad;
    std::vector<UserSample> m_userPath;
    std::size_t             m_userPathIndex;
    UserSample              m_lastUserSample;

    // ===== 变步长缓冲 =====
    struct InputSnapshot {
        double roll, pitch, yaw;
        double radarX, radarY, radarZ;
        double carrierFreqIn;
    };
    std::vector<InputSnapshot> m_inputBuffer;

    struct OutputFrame {
        std::vector<SystemVueModelBuilder::DoubleMatrix> pos;
        std::vector<double> rcs;
    };
    std::queue<OutputFrame> m_outputQueue;

    // ===== 基本工具函数 =====
    static double m_get_array_value(const std::vector<double>& data, int size, int idx, double defval);
    static double m_get_array_broadcast(const std::vector<double>& data, int size, int idx, double defval);

    static Vec3  m_make_vec(double x, double y, double z);
    static Vec3  m_add(const Vec3& a, const Vec3& b);
    static Vec3  m_sub(const Vec3& a, const Vec3& b);
    static Vec3  m_scale(const Vec3& a, double s);
    static double m_dot(const Vec3& a, const Vec3& b);
    static double m_norm(const Vec3& a);
    static Vec3  m_normalize(const Vec3& a, const Vec3& fallback);

    static Mat3 m_identity_mat();
    static Mat3 m_mat_mul(const Mat3& A, const Mat3& B);
    static Vec3 m_mat_vec(const Mat3& A, const Vec3& v);
    static Mat3 m_transpose(const Mat3& A);
    static Mat3 m_rot_x(double a);
    static Mat3 m_rot_y(double a);
    static Mat3 m_rot_z(double a);
    static Mat3 m_rpy_matrix(double rollRad, double pitchRad, double yawRad);

    static Vec3 m_lla_to_ecef(double lonRad, double latRad, double h);
    static Mat3 m_ned_to_ecef_matrix(const Vec3& north, const Vec3& east, const Vec3& up);
    static Mat3 m_ecef_to_eci_matrix(double theta);

    static double m_safe_log10(double x);
    static double m_clamp(double x, double lo, double hi);

    // ===== 算法辅助函数 =====
    bool m_validate_params() const;
    bool m_load_user_file();
    void m_init_empty_user_sample();

    Vec3   m_get_corner_loc(int idx) const;
    Vec3   m_get_phase_center_offset(int idx) const;
    double m_get_edge_length(int idx) const;
    double m_get_efficiency(int idx) const;

    double m_get_carrier_freq(double inputFreq);
    Vec3   m_get_radar_position(double rx, double ry, double rz);
    double m_peak_rcs_linear(int idx, double freqHz) const;

    double m_calc_corner_rcs(int idx,
        const Vec3& reflectorPos,
        const Mat3& bodyToGlobal,
        double freqHz,
        double radarX, double radarY, double radarZ);

    void m_compute_simple_xyz_center_and_orientation(
        Vec3& center, Mat3& bodyToGlobal,
        double rollDeg, double pitchDeg, double yawDeg);

    void m_compute_eci_center_and_orientation(
        Vec3& center, Mat3& bodyToGlobal,
        double rollDeg, double pitchDeg, double yawDeg);

    // ===== 辅助：算法参数 → m_algo =====
    void SetAlgoParameters();
};

RegAlgo(RADAR_CornerReflectorLocation_Block);

#endif // RADAR_CORNERREFLECTORLOCATION_BLOCK_H
