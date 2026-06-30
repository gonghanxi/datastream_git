#pragma once

#include "ModelBuilder.h"
#include "DFModel.h"
#include "CircularBuffer.h"
#include "Matrix.h"
#include "MatrixCircularBuffer.h"

#include <cmath>
#include <vector>
#include <string>

// 与 SystemVue 2020 内置 RADAR_Platform 黑盒结果对齐的等效实现。
// 域：Untimed Data Flow。
// 端口顺序按内置帮助文档组织：
//   输出端口 1：Pos，real matrix，3x1，Pos(0,0)=X、Pos(1,0)=Y、Pos(2,0)=Z
//   输入端口 2：Roll，可选 real
//   输入端口 3：Pitch，可选 real
//   输入端口 4：Yaw，可选 real
class RADAR_Platform : public SystemVueModelBuilder::DFModel
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
		IRE_false = 0,
		IRE_true = 1
	};
	enum PrintLogEnum
	{
		PrintLog_No = 0,
		PrintLog_Yes = 1
	};

	DECLARE_MODEL_INTERFACE(RADAR_Platform);
	RADAR_Platform();

	bool Setup() override;
	bool Run() override;

	// --------- 输出端口 ---------
	// 内置模型 Pos 的 Signal Type 是 real matrix。
	// 每次 Run() 输出 1 个 3x1 的 DoubleMatrix token，而不是 3 个 double token。
	SystemVueModelBuilder::DoubleMatrixCircularBuffer Pos;

	// --------- 输入端口 ---------
	// 三个输入端口均为可选蓝色浮点端口。未连接时黑盒结果等效为 0。
	// 声明/添加顺序保持为 Roll、Pitch、Yaw，以匹配内置端口号 2、3、4。
	SystemVueModelBuilder::CircularBuffer<double> Roll;
	SystemVueModelBuilder::CircularBuffer<double> Pitch;
	SystemVueModelBuilder::CircularBuffer<double> Yaw;

	// --------- 公共枚举参数 ---------
	Trajectory_ModeEnum Trajectory_Mode;
	PrintLogEnum        PrintLog;
	IsRandomErrorEnum   IsRandomError;

	// --------- ECI_Frame 参数 ---------
	// Position_Initial = [longitude(deg), latitude(deg), height(m)]
	double* Position_Initial;
	int     Position_InitialSize;

	double Velocity_Initial;
	double Accelerate_Initial;
	//bool   IsRandomError;
	double Accelerate_Variance;

	// --------- User Defined 参数 ---------
	char* FileName;

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

	static const double kPi;
	static const double kDegToRad;

	// 黑盒对齐：
	// 内置模型更接近使用 a=6378137.0、b=6356752.0 的截断椭球，
	// 而不是完整 WGS84 b=6356752.314245...。
	static const double kEarthSemiMajorAxis;
	static const double kEarthSemiMinorAxis;

	// 黑盒对齐：
	// 使用更高精度地球自转角速度，减小 ECI_Frame 中随采样点累积的旋转误差。
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
	std::vector<Vec3> userPath_;
	std::size_t userPathIndex_;
	Vec3 lastUserPos_;

	static double get_array_value_(const double* p, int size, int idx, double defval);

	static Vec3 make_vec_(double x, double y, double z);
	static Vec3 add_(const Vec3& a, const Vec3& b);
	static Vec3 scale_(const Vec3& a, double s);
	static double dot_(const Vec3& a, const Vec3& b);

	static Vec3 lla_to_ecef_(double lonRad, double latRad, double h);
	static Vec3 rotate_z_(const Vec3& v, double theta);

	// 写出 3x1 real matrix。若你的 SystemVue 2020 Matrix 下标接口为 1 基，
	// 只需在 cpp 中修改 write_pos_() 这一处。
	void write_pos_(double x, double y, double z);

	bool load_user_file_();
	double base_random_(unsigned long long k) const;
};
