#pragma once

#include "ModelBuilder.h"
#include "DFModel.h"
#include "CircularBuffer.h"

#include <cmath>
#include <limits>

// 与 SystemVue 内置 RADAR_TargetTrajectory 黑盒对齐的等效实现。
// 域：Untimed Data Flow。每次 Run() 在每个输出端口产生一个 token。
class SYSTEMVUEMODELBUILDER_API RADAR_TargetTrajectory : public SystemVueModelBuilder::DFModel
{
public:
	enum Coordinate_ModeEnum
	{
		Spherical = 0,
		Cartesian = 1
	};

	DECLARE_MODEL_INTERFACE(RADAR_TargetTrajectory);
	RADAR_TargetTrajectory();

	bool Setup() override;
	bool Run()   override;

	// --------- 输出端口 ---------
	// 以下五个输出在内置模型中为可选输出。
	// 声明和添加顺序保持为：Z、Y、X、Az、El、Delay、Range。
	SystemVueModelBuilder::CircularBuffer<double> Target_Pos_Z;
	SystemVueModelBuilder::CircularBuffer<double> Target_Pos_Y;
	SystemVueModelBuilder::CircularBuffer<double> Target_Pos_X;
	SystemVueModelBuilder::CircularBuffer<double> Target_Az_Angle;
	SystemVueModelBuilder::CircularBuffer<double> Target_El_Angle;

	// 以下两个输出在内置模型中为必选输出。
	SystemVueModelBuilder::CircularBuffer<double> Delay_Output;
	SystemVueModelBuilder::CircularBuffer<double> Range_Output;

	// --------- 参数 ---------
	Coordinate_ModeEnum Coordinate_Mode;

	// Spherical 模式参数。
	double Range_Initial;
	double ElevationAngle;
	double AzimuthAngle;
	double Velocity_Initial;
	double Accelerate;
	double Jerk;

	// Cartesian 模式数组参数。
	// 使用 double* + int Size，与已经编译通过的 BCH_Decoder 数组参数风格一致。
	// 数组内存由 SystemVue 仿真器管理，本模型只读取指针和长度。
	double* Position_Initial_XYZ;
	int     Position_Initial_XYZSize;

	double* Velocity_Initial_XYZ;
	int     Velocity_Initial_XYZSize;

	double* Accelerate_XYZ;
	int     Accelerate_XYZSize;

	double* Jerk_XYZ;
	int     Jerk_XYZSize;

	// 两种坐标模式共用的时间步长参数。
	double TimeStep;

private:
	static const double kPi;
	static const double kLightSpeed;
	static const double kSphericalInvalidXYZ;

	// 内部样本计数器。第 0 次 Run() 对应 t=0。
	unsigned long long sampleIndex_;

	static double get_array_value_(const double* p, int size, int idx, double defval);
	static double calc_az_(double x, double y);
	static double calc_el_(double x, double y, double z);
};
