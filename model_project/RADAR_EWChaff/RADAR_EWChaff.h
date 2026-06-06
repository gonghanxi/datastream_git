#pragma once

#include "ModelBuilder.h"
#include "DFModel.h"
#include "CircularBuffer.h"
#include "Matrix.h"
#include "MatrixCircularBuffer.h"

#include <cmath>
#include <vector>
#include <string>
#include <cstddef>

// ============================================================================
// RADAR_EWChaff
// ----------------------------------------------------------------------------
// 雷达电子战箔条干扰等效云模型。
//
// 本模型不逐根模拟每根箔条，而是把箔条云等效为若干个散射单元。
// 每个等效单元输出：位置、RCS、速度、有效标志。
// 后续回波生成模型可根据 ChaffPos + ChaffRCS + ChaffVel 生成雷达回波。
//
// 8 输入接口版本：
//   1 Release       : 箔条释放触发，>0 触发释放
//   2 PlatformX     : 释放平台 X 坐标
//   3 PlatformY     : 释放平台 Y 坐标
//   4 PlatformZ     : 释放平台 Z 坐标
//   5 PlatformVx    : 释放平台 X 向速度
//   6 PlatformVy    : 释放平台 Y 向速度
//   7 PlatformVz    : 释放平台 Z 向速度
//   8 CarrierFreqIn : 动态载频输入
// ============================================================================

class SYSTEMVUEMODELBUILDER_API RADAR_EWChaff : public SystemVueModelBuilder::DFModel
{
public:
	enum Chaff_ModeEnum
	{
		FixedCloud = 0,
		ReleasedCloud = 1,
		User_Defined = 2
	};

	enum Cloud_ModelEnum
	{
		SingleCell = 0,
		MultiCell = 1
	};

	enum RCSModelEnum
	{
		// 经验总 RCS：TotalRCS_Reference 作为完全展开后的峰值总 RCS。
		EmpiricalTotalRCS = 0,

		// 半波偶极子近似：根据 DipoleLength 与 lambda/2 的接近程度修正 RCS。
		ResonantDipoleApprox = 1
	};

	enum RCSOutputUnitEnum
	{
		Linear_m2 = 0,
		dBsm = 1
	};

	enum CellRCSDistributionEnum
	{
		Uniform_Distribution = 0,
		GaussianCenter_Distribution = 1
	};

	DECLARE_MODEL_INTERFACE(RADAR_EWChaff);
	RADAR_EWChaff();

	bool Setup() override;
	bool Run() override;

	// --------- 输入端口 ---------
	SystemVueModelBuilder::CircularBuffer<double> Release;

	SystemVueModelBuilder::CircularBuffer<double> PlatformX;
	SystemVueModelBuilder::CircularBuffer<double> PlatformY;
	SystemVueModelBuilder::CircularBuffer<double> PlatformZ;

	SystemVueModelBuilder::CircularBuffer<double> PlatformVx;
	SystemVueModelBuilder::CircularBuffer<double> PlatformVy;
	SystemVueModelBuilder::CircularBuffer<double> PlatformVz;

	SystemVueModelBuilder::CircularBuffer<double> CarrierFreqIn;

	// --------- 输出端口 ---------
	SystemVueModelBuilder::CircularBufferBusT<
		SystemVueModelBuilder::DoubleMatrixCircularBuffer
	> ChaffPos;

	SystemVueModelBuilder::CircularBufferBusT<
		SystemVueModelBuilder::CircularBuffer<double>
	> ChaffRCS;

	SystemVueModelBuilder::CircularBufferBusT<
		SystemVueModelBuilder::DoubleMatrixCircularBuffer
	> ChaffVel;

	SystemVueModelBuilder::CircularBufferBusT<
		SystemVueModelBuilder::CircularBuffer<double>
	> ValidFlag;

	// --------- 模式参数 ---------
	Chaff_ModeEnum Chaff_Mode;
	Cloud_ModelEnum Cloud_Model;
	RCSModelEnum RCS_Model;
	RCSOutputUnitEnum RCS_OutputUnit;
	CellRCSDistributionEnum Cell_RCS_Distribution;

	int NumberOfChaffCell;

	// --------- UserDefined 参数 ---------
	// 文件每行格式：
	//   x0 y0 z0 rcs0 vx0 vy0 vz0 valid0 x1 y1 z1 rcs1 vx1 vy1 vz1 valid1 ...
	// 每 8 个数表示一个箔条云等效单元。
	char* FileName;

	// --------- 释放点/备用参数 ---------
	// Release_Position_XYZ：
	//   FixedCloud 模式下作为固定云团中心；
	//   ReleasedCloud 模式下，如果 PlatformX/Y/Z 未连接，则作为释放点备用值。
	double* Release_Position_XYZ;
	int     Release_Position_XYZSize;

	// Initial_Velocity_XYZ：
	//   ReleasedCloud 模式下，如果 PlatformVx/Vy/Vz 未连接，则作为初始速度备用值。
	double* Initial_Velocity_XYZ;
	int     Initial_Velocity_XYZSize;

	// 风速参数，单位 m/s。
	double* Wind_Velocity_XYZ;
	int     Wind_Velocity_XYZSize;

	// 平均下沉速度，正值表示沿 -Z 方向下沉。
	double Fall_Speed;

	// 平台初速度记忆衰减时间常数。
	// 刚释放时箔条继承平台速度，随后因阻力逐渐失速并接近风速。
	double VelocityDecayTime;

	// --------- 云团扩散参数 ---------
	double* Cloud_Initial_Radius_XYZ;
	int     Cloud_Initial_Radius_XYZSize;

	double* Cloud_ExpansionRate_XYZ;
	int     Cloud_ExpansionRate_XYZSize;

	double* Cloud_MaxRadius_XYZ;
	int     Cloud_MaxRadius_XYZSize;

	double Cloud_Lifetime;

	// --------- 箔条物理/RCS 参数 ---------
	double CarrierFreq;
	double DipoleLength;
	double DipoleLengthSpread;
	double NumberOfDipoles;
	double ReferenceDipoleCount;

	double TotalRCS_Reference;
	double RCS_GrowthTime;
	double RCS_DecayTime;
	double RCS_Floor;

	double GaussianWeightSigma;

	// --------- 时间参数 ---------
	double TimeStep;

private:
	struct Vec3
	{
		double x;
		double y;
		double z;
	};

	struct UserSample
	{
		std::vector<Vec3> pos;
		std::vector<double> rcs;
		std::vector<Vec3> vel;
		std::vector<double> valid;
	};

	static const double kPi;
	static const double kSpeedOfLight;
	static const double kGoldenAngle;

	unsigned long long sampleIndex_;

	bool released_;
	unsigned long long releaseSampleIndex_;

	// 释放瞬间锁存平台位置和平台速度。
	Vec3 releasePosLatched_;
	Vec3 releaseVelLatched_;

	std::vector<UserSample> userPath_;
	std::size_t userPathIndex_;
	UserSample lastUserSample_;

	static double get_array_value_(const double* p, int size, int idx, double defval);
	static Vec3 get_array_vec3_(const double* p, int size, double defx, double defy, double defz);

	static Vec3 make_vec_(double x, double y, double z);
	static Vec3 add_(const Vec3& a, const Vec3& b);
	static Vec3 sub_(const Vec3& a, const Vec3& b);
	static Vec3 scale_(const Vec3& a, double s);
	static Vec3 mul_(const Vec3& a, const Vec3& b);
	static double dot_(const Vec3& a, const Vec3& b);
	static double norm_(const Vec3& a);
	static Vec3 normalize_(const Vec3& a, const Vec3& fallback);
	static double safe_log10_(double x);

	int active_cell_count_() const;
	bool validate_params_() const;

	bool load_user_file_();
	void init_empty_user_sample_();

	double get_carrier_freq_();

	Vec3 get_platform_position_();
	Vec3 get_platform_velocity_();

	Vec3 get_release_position_param_() const;
	Vec3 get_initial_velocity_param_() const;
	Vec3 get_wind_velocity_() const;
	Vec3 get_initial_radius_() const;
	Vec3 get_expansion_rate_() const;
	Vec3 get_max_radius_() const;

	void compute_cloud_center_velocity_radius_(double age,
		Vec3& center,
		Vec3& centerVel,
		Vec3& radius,
		Vec3& radiusRate) const;

	Vec3 cell_pattern_(int idx, int n) const;

	void compute_cell_weights_(const std::vector<Vec3>& patterns,
		std::vector<double>& weights) const;

	double frequency_match_factor_(double freqHz) const;
	double total_rcs_linear_(double age, double freqHz, bool active) const;

	void write_outputs_(const std::vector<Vec3>& positions,
		const std::vector<double>& rcsValues,
		const std::vector<Vec3>& velocities,
		const std::vector<double>& validValues);
};
