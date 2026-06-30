#ifndef RADAR_PLATFORM_BLOCK_H
#define RADAR_PLATFORM_BLOCK_H

#include "RADAR_Platform.h"
#include "Block.h"
#include "Matrix.h"

#include <memory>
#include <string>
#include <vector>
#include <deque>
#include <queue>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_Platform_Block : public Block
{
public:
    RADAR_Platform_Block(const std::string& name);
    ~RADAR_Platform_Block() = default;

    bool Setup() override;
    bool Initialize() override;
    bool Run() override;

    void SetDefaultParameters();
    void SetParameters();

private:
    bool ModelSetup();
    bool DataStreamRun();
    bool TimeDrivenRun();

    // 仅用于端口注册的算法实例
    std::unique_ptr<RADAR_Platform> m_algo;

    // ========== 枚举 ==========
    enum TrajectoryMode {
        ECI_Frame = 0,
        User_Defined = 1,
        SimpleXYZ_Frame = 2
    };
    enum IsRandomErrorEnum { IRE_false = 0, IRE_true = 1 };
    enum PrintLogEnum { PrintLog_No = 0, PrintLog_Yes = 1 };

    struct Vec3 { double x, y, z; };

    // ========== 参数 ==========
    TrajectoryMode Trajectory_Mode_;
    PrintLogEnum PrintLog_;
    IsRandomErrorEnum IsRandomError_;

    // ECI_Frame 参数
    std::vector<double> Position_Initial_;
    double Velocity_Initial_;
    double Accelerate_Initial_;
    double Accelerate_Variance_;

    // User Defined 参数
    std::string FileName_;

    // SimpleXYZ_Frame 参数
    std::vector<double> Position_Initial_XYZ_;
    std::vector<double> Velocity_Initial_XYZ_;
    std::vector<double> Accelerate_XYZ_;
    std::vector<double> Jerk_XYZ_;

    // 公共时间参数
    double TimeStep_;

    // ========== 运行时状态 ==========
    unsigned long long sampleIndex_;

    // ECI 模式内部状态
    Vec3 p0Ecef_;
    Vec3 eastEcef_;
    Vec3 northEcef_;
    Vec3 upEcef_;
    Vec3 motionAccumEcef_;
    double lonRad_;
    double latRad_;

    // User Defined 模式内部状态
    std::vector<Vec3> userPath_;
    std::size_t userPathIndex_;
    Vec3 lastUserPos_;

    // ========== 时间驱动模式缓冲区 ==========
    std::queue<DoubleMatrix> m_outputQueue;

    // ========== 核心计算 ==========
    DoubleMatrix computePosition_();
    DoubleMatrix computeECI_(double rollDeg, double pitchDeg, double yawDeg);
    DoubleMatrix computeSimpleXYZ_();
    DoubleMatrix computeUserDefined_();

    // ========== 辅助函数 ==========
    bool loadUserFile_();
    double baseRandom_(unsigned long long k) const;

    static Vec3 makeVec_(double x, double y, double z);
    static Vec3 add_(const Vec3& a, const Vec3& b);
    static Vec3 scale_(const Vec3& a, double s);
    static double dot_(const Vec3& a, const Vec3& b);
    static Vec3 llaToEcef_(double lonRad, double latRad, double h);
    static Vec3 rotateZ_(const Vec3& v, double theta);
    static double getArrayValue_(const std::vector<double>& v, int idx, double defval);

    // ========== 参数解析 ==========
    static TrajectoryMode ParseTrajectoryMode(const std::string& str);
    static IsRandomErrorEnum ParseIsRandomError(const std::string& str);
    static PrintLogEnum ParsePrintLog(const std::string& str);
    static std::vector<double> ParseDoubleArray(const std::string& str);

    static constexpr double kPi = 3.1415926535897932384626433832795;
    static constexpr double kDegToRad = kPi / 180.0;
    static constexpr double kEarthSemiMajorAxis = 6378137.0;
    static constexpr double kEarthSemiMinorAxis = 6356752.0;
    static constexpr double kEarthRotationRate = 7.2921151467e-5;
};

RegAlgo(RADAR_Platform_Block);

#endif // RADAR_PLATFORM_BLOCK_H
