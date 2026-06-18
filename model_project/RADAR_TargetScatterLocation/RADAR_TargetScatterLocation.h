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

// 与 SystemVue 2020 内置 RADAR_TargetScatterLocation 黑盒结果对齐的等效实现。
// 域：Untimed Data Flow。
// 端口顺序按内置帮助文档组织：
//   输入端口 1：Roll，可选 real
//   输入端口 2：Pitch，可选 real
//   输入端口 3：Yaw，可选 real
//   输出端口 4：Pos，multiple real matrix，每个 scatter 一个 3x1 matrix token
//   输出端口 5：ScatterRCS，multiple real，每个 scatter 一个 real token
class SYSTEMVUEMODELBUILDER_API RADAR_TargetScatterLocation : public SystemVueModelBuilder::DFModel
{
public:
	enum Trajectory_ModeEnum
	{
		ECI_Frame = 0,
		User_Defined = 1,
		SimpleXYZ_Frame = 2
	};

	enum IsRandomErrorEnum
	{
		RandomError_false = 0,
		RandomError_true = 1
	};

	enum IsRCSRandomEnum
	{
		RCSRandom_false = 0,
		RCSRandom_true = 1
	};

	DECLARE_MODEL_INTERFACE(RADAR_TargetScatterLocation);
	RADAR_TargetScatterLocation();

	bool Setup() override;
	bool Run() override;

	// --------- 输入端口 ---------
	// 三个输入端口均为可选蓝色浮点端口。黑盒结果表明：
	// Roll/Pitch/Yaw 不旋转 ScatterLoc；在 ECI_Frame 中 Yaw/Pitch 只影响目标质心运动方向。
	SystemVueModelBuilder::CircularBuffer<double> Roll;
	SystemVueModelBuilder::CircularBuffer<double> Pitch;
	SystemVueModelBuilder::CircularBuffer<double> Yaw;

	// --------- 输出端口 ---------
	// 内置模型输出为 bus：
	//   Pos        : multiple real matrix，每个 bus 通道为一个 scatter 的 3x1 DoubleMatrix。
	//   ScatterRCS : multiple real，每个 bus 通道为一个 scatter 的 RCS 标量。
	// 注意：不能用 SetRate(NumberOfTargetScatter) 模拟 bus，否则 SystemVue 不会识别为 multiple bus。
	SystemVueModelBuilder::CircularBufferBusT<
		SystemVueModelBuilder::DoubleMatrixCircularBuffer
	> Pos;

	SystemVueModelBuilder::CircularBufferBusT<
		SystemVueModelBuilder::CircularBuffer<double>
	> ScatterRCS;

	// --------- 公共参数 ---------
	Trajectory_ModeEnum Trajectory_Mode;
	int                 NumberOfTargetScatter;

	// --------- User Defined 参数 ---------
	char* FileName;

	// --------- ECI_Frame 与 SimpleXYZ_Frame 共用散射点/RCS参数 ---------
	double* ScatterLoc;
	int     ScatterLocSize;

	double* RCS;
	int     RCSSize;

	IsRCSRandomEnum IsRCSRandom;

	double* RCS_Variance;
	int     RCS_VarianceSize;

	// 帮助文档中 Type 显示为 Float，但黑盒结果表明 DurationTime 可按数组使用，
	// 且不会自动广播到所有 scatter，因此这里按数组参数实现。
	double* DurationTime;
	int     DurationTimeSize;

	// --------- ECI_Frame 参数 ---------
	// Position_Initial = [longitude(deg), latitude(deg), height(m)]
	double* Position_Initial;
	int     Position_InitialSize;

	double Velocity_Initial;
	double Accelerate_Initial;
	IsRandomErrorEnum IsRandomError;
	double Accelerate_Variance;

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

	struct UserSample
	{
		std::vector<Vec3> pos;
		std::vector<double> rcs;
	};

	static const double kPi;
	static const double kDegToRad;
	static const double kEarthSemiMajorAxis;
	static const double kEarthSemiMinorAxis;
	static const double kEarthRotationRate;

	unsigned long long sampleIndex_;

	// ECI 模式内部状态。
	Vec3 p0Ecef_;
	Vec3 eastEcef_;
	Vec3 northEcef_;
	Vec3 upEcef_;
	Vec3 motionAccumEcef_;
	double lonRad_;
	double latRad_;

	// User Defined 模式内部状态。
	std::vector<UserSample> userPath_;
	std::size_t userPathIndex_;
	UserSample lastUserSample_;

	static double get_array_value_(const double* p, int size, int idx, double defval);
	static Vec3 make_vec_(double x, double y, double z);
	static Vec3 add_(const Vec3& a, const Vec3& b);
	static Vec3 scale_(const Vec3& a, double s);
	static double dot_(const Vec3& a, const Vec3& b);

	static Vec3 lla_to_ecef_(double lonRad, double latRad, double h);
	static Vec3 rotate_z_(const Vec3& v, double theta);

	bool validate_params_() const;
	bool load_user_file_();
	void init_empty_user_sample_();

	Vec3 get_scatter_loc_(int idx) const;
	double get_rcs_base_(int idx) const;
	double calc_scatter_rcs_(int idx, unsigned long long k) const;

	double base_random_position_(unsigned long long k) const;
	double base_random_rcs_(unsigned long long blockIndex, int scatterIndex) const;

	void write_outputs_(const std::vector<Vec3>& positions, const std::vector<double>& rcsValues);
};
