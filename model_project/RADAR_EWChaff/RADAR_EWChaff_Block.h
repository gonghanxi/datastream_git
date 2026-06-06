#ifndef RADAR_EWCHAFF_BLOCK_H
#define RADAR_EWCHAFF_BLOCK_H

#include "Block.h"
#include "RADAR_EWChaff.h"

#include <memory>
#include <queue>
#include <string>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_EWChaff_Block : public SystemVueModelBuilder::Block
{
public:
    RADAR_EWChaff_Block(const std::string& name);
    ~RADAR_EWChaff_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

private:
    void SetDefaultParameters();

    bool DataStreamRun();
    bool TimeDrivenRun();

    // ===== 字符串转换（5 个枚举） =====
    RADAR_EWChaff::Chaff_ModeEnum          ConvertStringToChaffMode(const std::string& value);
    RADAR_EWChaff::Cloud_ModelEnum         ConvertStringToCloudModel(const std::string& value);
    RADAR_EWChaff::RCSModelEnum            ConvertStringToRCSModel(const std::string& value);
    RADAR_EWChaff::RCSOutputUnitEnum       ConvertStringToRCSOutputUnit(const std::string& value);
    RADAR_EWChaff::CellRCSDistributionEnum ConvertStringToCellRCSDistribution(const std::string& value);

    // ===== 算法对象（仅用于端口注册和 Setup） =====
    std::unique_ptr<RADAR_EWChaff> m_algo;

    // ===== 枚举参数 =====
    RADAR_EWChaff::Chaff_ModeEnum          m_Chaff_Mode;
    RADAR_EWChaff::Cloud_ModelEnum         m_Cloud_Model;
    RADAR_EWChaff::RCSModelEnum            m_RCS_Model;
    RADAR_EWChaff::RCSOutputUnitEnum       m_RCS_OutputUnit;
    RADAR_EWChaff::CellRCSDistributionEnum m_Cell_RCS_Distribution;

    int    m_NumberOfChaffCell;

    // ===== UserDefined 参数 =====
    std::string m_FileName;

    // ===== 指针数组参数（三件套） =====
    std::vector<double> m_Release_Position_XYZData;
    std::vector<double> m_Initial_Velocity_XYZData;
    std::vector<double> m_Wind_Velocity_XYZData;
    std::vector<double> m_Cloud_Initial_Radius_XYZData;
    std::vector<double> m_Cloud_ExpansionRate_XYZData;
    std::vector<double> m_Cloud_MaxRadius_XYZData;

    // ===== 标量参数 =====
    double m_Fall_Speed;
    double m_VelocityDecayTime;
    double m_Cloud_Lifetime;
    double m_CarrierFreq;
    double m_DipoleLength;
    double m_DipoleLengthSpread;
    double m_NumberOfDipoles;
    double m_ReferenceDipoleCount;
    double m_TotalRCS_Reference;
    double m_RCS_GrowthTime;
    double m_RCS_DecayTime;
    double m_RCS_Floor;
    double m_GaussianWeightSigma;
    double m_TimeStep;

    // ===== 算法内部状态 =====
    struct Vec3 { double x; double y; double z; };
    struct UserSample {
        std::vector<Vec3> pos;
        std::vector<double> rcs;
        std::vector<Vec3> vel;
        std::vector<double> valid;
    };

    unsigned long long m_sampleIndex;
    bool               m_released;
    unsigned long long m_releaseSampleIndex;
    Vec3               m_releasePosLatched;
    Vec3               m_releaseVelLatched;
    std::vector<UserSample> m_userPath;
    std::size_t             m_userPathIndex;
    UserSample              m_lastUserSample;

    // ===== 变步长输入缓冲 & 输出队列 =====
    struct InputSnapshot {
        double release, platX, platY, platZ;
        double platVx, platVy, platVz, carrierFreqIn;
    };
    std::vector<InputSnapshot> m_inputBuffer;

    struct ChaffOutput {
        std::vector<SystemVueModelBuilder::DoubleMatrix> pos;
        std::vector<double> rcs;
        std::vector<SystemVueModelBuilder::DoubleMatrix> vel;
        std::vector<double> valid;
    };
    std::queue<ChaffOutput> m_outputQueue;

    // ===== 算法辅助函数 =====
    static double m_get_array_value(const std::vector<double>& data, int idx, double defval);
    static Vec3   m_get_array_vec3(const std::vector<double>& data, double defx, double defy, double defz);

    static Vec3 m_make_vec(double x, double y, double z);
    static Vec3 m_add(const Vec3& a, const Vec3& b);
    static Vec3 m_sub(const Vec3& a, const Vec3& b);
    static Vec3 m_scale(const Vec3& a, double s);
    static Vec3 m_mul(const Vec3& a, const Vec3& b);
    static double m_dot(const Vec3& a, const Vec3& b);
    static double m_norm(const Vec3& a);
    static Vec3 m_normalize(const Vec3& a, const Vec3& fallback);
    static double m_safe_log10(double x);

    int m_active_cell_count() const;
    bool m_validate_params() const;

    double m_get_carrier_freq();
    Vec3 m_get_platform_position();
    Vec3 m_get_platform_velocity();
    Vec3 m_get_release_position_param() const;
    Vec3 m_get_initial_velocity_param() const;
    Vec3 m_get_wind_velocity() const;
    Vec3 m_get_initial_radius() const;
    Vec3 m_get_expansion_rate() const;
    Vec3 m_get_max_radius() const;

    void m_compute_cloud(double age, Vec3& center, Vec3& centerVel, Vec3& radius, Vec3& radiusRate) const;
    Vec3 m_cell_pattern(int idx, int n) const;
    void m_compute_cell_weights(const std::vector<Vec3>& patterns, std::vector<double>& weights) const;
    double m_frequency_match_factor(double freqHz) const;
    double m_total_rcs_linear(double age, double freqHz, bool active) const;

    // ===== m_algo 参数设置（用于 Setup） =====
    void SetAlgoParameters();
};

RegAlgo(RADAR_EWChaff_Block);

#endif // RADAR_EWCHAFF_BLOCK_H
