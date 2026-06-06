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
// RADAR_CornerReflectorLocation
// ----------------------------------------------------------------------------
// 支持的第一版物理模型：
//   1) 三角形三面角反射器：sigma_peak = 4*pi*a^4 / (3*lambda^2)
//   2) 正方形三面角反射器：sigma_peak = 12*pi*a^4 / lambda^2
//
// 其中：
//   a      : 三面角反射器内部边长，单位 m
//   lambda : 雷达波长，lambda = c / f
//   sigma  : RCS，单位 m^2
//
// 坐标约定：
//   body frame 下，三块反射面可理解为 x=0, y=0, z=0 三个正交面；
//   角反射器开口方向位于 +x,+y,+z 象限；
//   boresight 方向为 normalize([1,1,1])。
// ============================================================================

class SYSTEMVUEMODELBUILDER_API RADAR_CornerReflectorLocation : public SystemVueModelBuilder::DFModel
{
public:
	enum Trajectory_ModeEnum
	{
		ECI_Frame = 0,
		User_Defined = 1,
		SimpleXYZ_Frame = 2
	};

	// 运动模式：
	// Fixed_Mode 只使用初始位置，角反射器保持固定；
	// Moving_Mode 才启用速度、加速度和加加速度参数。
	enum Motion_ModeEnum
	{
		Fixed_Mode = 0,
		Moving_Mode = 1
	};

	enum ReflectorTypeEnum
	{
		Triangular_Trihedral = 0,
		Square_Trihedral = 1
	};

	enum RCSModelEnum
	{
		// 只输出理论峰值 RCS，等价于假设雷达正对角反射器 boresight。
		PeakOnly = 0,

		// 在 PeakOnly 基础上增加开口方向和 boresight 锥角门限。
		// 若雷达不在角反射器开口方向内，或偏离 boresight 超过 BoresightHalfAngle，
		// 则输出 RCS_Floor。
		BoresightCone = 1
	};

	enum RCSOutputUnitEnum
	{
		Linear_m2 = 0,
		dBsm = 1
	};

	DECLARE_MODEL_INTERFACE(RADAR_CornerReflectorLocation);
	RADAR_CornerReflectorLocation();

	bool Setup() override;
	bool Run() override;

	// --------- 输入端口 ---------
	// 端口 1~3：角反射器姿态，单位 deg。
	// Roll/Pitch/Yaw 采用常见 R = Rz(yaw) * Ry(pitch) * Rx(roll) 主动旋转。
	SystemVueModelBuilder::CircularBuffer<double> Roll;
	SystemVueModelBuilder::CircularBuffer<double> Pitch;
	SystemVueModelBuilder::CircularBuffer<double> Yaw;

	// 端口 4~6：雷达位置，可选，单位 m。
	// 若未连接，则使用参数 Radar_Position_XYZ。
	// 在 SimpleXYZ_Frame 中，RadarX/Y/Z 与 Pos 同属 XYZ 坐标系。
	// 在 ECI_Frame 中，RadarX/Y/Z 认为已经是 ECI 坐标。
	SystemVueModelBuilder::CircularBuffer<double> RadarX;
	SystemVueModelBuilder::CircularBuffer<double> RadarY;
	SystemVueModelBuilder::CircularBuffer<double> RadarZ;

	// 端口 7：载频输入，可选，单位 Hz。
	// 若未连接或 <=0，则使用参数 CarrierFreq。
	SystemVueModelBuilder::CircularBuffer<double> CarrierFreqIn;

	// --------- 输出端口 ---------
	// Pos:
	//   multiple real matrix，每个 bus 通道输出一个角反射器位置，矩阵尺寸 3x1。
	// CornerRCS:
	//   multiple real，每个 bus 通道输出一个角反射器 RCS。
	SystemVueModelBuilder::CircularBufferBusT<
		SystemVueModelBuilder::DoubleMatrixCircularBuffer
	> Pos;

	SystemVueModelBuilder::CircularBufferBusT<
		SystemVueModelBuilder::CircularBuffer<double>
	> CornerRCS;

	// --------- 公共轨迹参数 ---------
	Trajectory_ModeEnum Trajectory_Mode;
	Motion_ModeEnum     Motion_Mode;
	int                 NumberOfCornerReflector;

	// --------- User Defined 参数 ---------
	// 文件模式下每行格式：
	//   x0 y0 z0 rcs0 x1 y1 z1 rcs1 ...
	// 每行对应一个采样时刻；读完文件后保持最后一行。
	char* FileName;

	// --------- 角反射器几何参数 ---------
	ReflectorTypeEnum ReflectorType;
	RCSModelEnum      RCS_Model;
	RCSOutputUnitEnum RCS_OutputUnit;

	// CornerLoc:
	//   每个角反射器相对模型质心的位置偏移，body frame 下定义，格式：
	//   [x0 y0 z0 x1 y1 z1 ...]，单位 m。
	double* CornerLoc;
	int     CornerLocSize;

	// EdgeLength:
	//   三面角反射器内部边长 a，单位 m。
	//   长度可以为 1，此时对所有角反射器广播；也可以为 N。
	double* EdgeLength;
	int     EdgeLengthSize;

	// Efficiency:
	//   反射效率，默认 1。用于粗略考虑加工误差、有限导电率、装配损耗。
	//   最终 sigma = Efficiency * sigma_ideal。
	double* Efficiency;
	int     EfficiencySize;

	// 每个角反射器的姿态偏置，单位 deg。
	// 用于一个模型里放多个朝向不同的角反射器。
	double* CornerRollOffset;
	int     CornerRollOffsetSize;

	double* CornerPitchOffset;
	int     CornerPitchOffsetSize;

	double* CornerYawOffset;
	int     CornerYawOffsetSize;

	// PhaseCenterOffset:
	//   等效相位中心相对角点/参考点的偏移，body frame 下定义，单位 m。
	//   第一版默认 [0 0 0]，因为不同结构的精确相位中心与安装方式、频率和视角有关。
	double* PhaseCenterOffset;
	int     PhaseCenterOffsetSize;

	// --------- RCS 计算参数 ---------
	double CarrierFreq;          // Hz
	double BoresightHalfAngle;   // deg，仅 BoresightCone 模式使用
	double RCS_Floor;            // m^2，线性最小 RCS，RCS_OutputUnit=dBsm 时内部仍按 m^2 计算

	// 雷达位置参数，作为 RadarX/Y/Z 未连接时的默认值。
	double* Radar_Position_XYZ;
	int     Radar_Position_XYZSize;

	// --------- ECI_Frame 参数 ---------
	// Position_Initial = [longitude(deg), latitude(deg), height(m)]
	double* Position_Initial;
	int     Position_InitialSize;

	double Velocity_Initial;      // m/s，沿 yaw/pitch 指定方向
	double Accelerate_Initial;    // m/s^2，沿 yaw/pitch 指定方向

	// --------- SimpleXYZ_Frame 参数 ---------
	double* Position_Initial_XYZ;
	int     Position_Initial_XYZSize;

	double* Velocity_Initial_XYZ;
	int     Velocity_Initial_XYZSize;

	double* Accelerate_XYZ;
	int     Accelerate_XYZSize;

	double* Jerk_XYZ;
	int     Jerk_XYZSize;

	// --------- 公共时间参数 ---------
	double TimeStep;

private:
	struct Vec3
	{
		double x;
		double y;
		double z;
	};

	struct Mat3
	{
		double m[3][3];
	};

	struct UserSample
	{
		std::vector<Vec3> pos;
		std::vector<double> rcs;
	};

	static const double kPi;
	static const double kDegToRad;
	static const double kRadToDeg;
	static const double kSpeedOfLight;
	static const double kEarthSemiMajorAxis;
	static const double kEarthSemiMinorAxis;
	static const double kEarthRotationRate;

	unsigned long long sampleIndex_;

	// ECI 模式内部状态。
	Vec3 p0Ecef_;
	Vec3 eastEcef_;
	Vec3 northEcef_;
	Vec3 upEcef_;
	double lonRad_;
	double latRad_;

	// User Defined 模式内部状态。
	std::vector<UserSample> userPath_;
	std::size_t userPathIndex_;
	UserSample lastUserSample_;

	static double get_array_value_(const double* p, int size, int idx, double defval);
	static double get_array_broadcast_(const double* p, int size, int idx, double defval);

	static Vec3 make_vec_(double x, double y, double z);
	static Vec3 add_(const Vec3& a, const Vec3& b);
	static Vec3 sub_(const Vec3& a, const Vec3& b);
	static Vec3 scale_(const Vec3& a, double s);
	static double dot_(const Vec3& a, const Vec3& b);
	static double norm_(const Vec3& a);
	static Vec3 normalize_(const Vec3& a, const Vec3& fallback);

	static Mat3 identity_mat_();
	static Mat3 mat_mul_(const Mat3& A, const Mat3& B);
	static Vec3 mat_vec_(const Mat3& A, const Vec3& v);
	static Mat3 transpose_(const Mat3& A);
	static Mat3 rot_x_(double a);
	static Mat3 rot_y_(double a);
	static Mat3 rot_z_(double a);
	static Mat3 rpy_matrix_(double rollRad, double pitchRad, double yawRad);

	static Vec3 lla_to_ecef_(double lonRad, double latRad, double h);
	static Mat3 ned_to_ecef_matrix_(const Vec3& north, const Vec3& east, const Vec3& up);
	static Mat3 ecef_to_eci_matrix_(double theta);

	static double safe_log10_(double x);
	static double clamp_(double x, double lo, double hi);

	bool validate_params_() const;
	bool load_user_file_();
	void init_empty_user_sample_();

	Vec3 get_corner_loc_(int idx) const;
	Vec3 get_phase_center_offset_(int idx) const;

	double get_edge_length_(int idx) const;
	double get_efficiency_(int idx) const;

	// 注意：下面几个函数会读取输入端口，不能声明为 const。
	double get_carrier_freq_();
	Vec3 get_radar_position_();

	double peak_rcs_linear_(int idx, double freqHz) const;
	double calc_corner_rcs_(int idx,
		const Vec3& reflectorPos,
		const Mat3& bodyToGlobal,
		double freqHz);

	void compute_simple_xyz_center_and_orientation_(Vec3& center, Mat3& bodyToGlobal);
	void compute_eci_center_and_orientation_(Vec3& center, Mat3& bodyToGlobal);

	void write_outputs_(const std::vector<Vec3>& positions,
		const std::vector<double>& rcsValues);
};
