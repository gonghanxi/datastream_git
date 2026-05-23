#include "RADAR_TargetTrajectory.h"

const double RADAR_TargetTrajectory::kPi = 3.1415926535897932384626433832795;
const double RADAR_TargetTrajectory::kLightSpeed = 300000000.0;
const double RADAR_TargetTrajectory::kSphericalInvalidXYZ = 1.0e-300;

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(RADAR_TargetTrajectory)
{
	SET_MODEL_DESCRIPTION("Target Trajectory");
	// SET_MODEL_SYMBOL("SYM_RADAR_TargetTrajectory");
	SET_MODEL_CATEGORY("Environments");

	// --------- 输出端口 ---------
	{
		auto p = ADD_MODEL_OUTPUT(Range_Output);
		p.SetDescription("instantaneous range");
	}
	{
		auto p = ADD_MODEL_OUTPUT(Delay_Output);
		p.SetDescription("instantaneous delay");
	}
	{
		auto p = ADD_MODEL_OUTPUT(Target_El_Angle);
		p.SetDescription("Target Elevation Angle relative to the phase center of the antenna");
		p.SetOptional(true);
	}
	{
		auto p = ADD_MODEL_OUTPUT(Target_Az_Angle);
		p.SetDescription("Target Azimuth Angle relative to the phase center of the antenna");
		p.SetOptional(true);
	}
	{
		auto p = ADD_MODEL_OUTPUT(Target_Pos_X);
		p.SetDescription("Target position value in cartesian coordinate system");
		p.SetOptional(true);
	}
	{
		auto p = ADD_MODEL_OUTPUT(Target_Pos_Y);
		p.SetDescription("Target position value in cartesian coordinate system");
		p.SetOptional(true);
	}
	{
		auto p = ADD_MODEL_OUTPUT(Target_Pos_Z);
		p.SetDescription("Target position value in cartesian coordinate system");
		p.SetOptional(true);
	}

	// --------- 坐标模式参数 ---------
	{
		auto p = ADD_MODEL_ENUM_PARAM(Coordinate_Mode, Coordinate_ModeEnum);
		p.AddEnumeration("Spherical", RADAR_TargetTrajectory::Spherical);
		p.AddEnumeration("Cartesian", RADAR_TargetTrajectory::Cartesian);
		p.SetDefaultValue("Spherical");
		p.SetDescription("The Coordinate Mode");
	}

	// --------- Spherical 模式参数 ---------
	{
		auto p = ADD_MODEL_PARAM(Range_Initial);
		p.SetDefaultValue("20e3");
		p.SetUnit(SystemVueModelBuilder::Units::LENGTH);
		p.SetDescription("The initial range in antenna boresight direction");
		p.SetHideCondition("Coordinate_Mode ~= 0");
	}
	{
		auto p = ADD_MODEL_PARAM(ElevationAngle);
		p.SetDefaultValue("0");
		p.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		p.SetDescription("The elevation angle between target and antenna boresight direction");
		p.SetHideCondition("Coordinate_Mode ~= 0");
	}
	{
		auto p = ADD_MODEL_PARAM(AzimuthAngle);
		p.SetDefaultValue("0");
		p.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		p.SetDescription("The azimuth angle between target and antenna boresight direction");
		p.SetHideCondition("Coordinate_Mode ~= 0");
	}
	{
		auto p = ADD_MODEL_PARAM(Velocity_Initial);
		p.SetDefaultValue("0");
		p.SetDescription("The initial velocity of the target");
		p.SetHideCondition("Coordinate_Mode ~= 0");
	}
	{
		auto p = ADD_MODEL_PARAM(Accelerate);
		p.SetDefaultValue("0");
		p.SetDescription("The accelerate of the target");
		p.SetHideCondition("Coordinate_Mode ~= 0");
	}
	{
		auto p = ADD_MODEL_PARAM(Jerk);
		p.SetDefaultValue("0");
		p.SetDescription("The jerk of the target");
		p.SetHideCondition("Coordinate_Mode ~= 0");
	}

	// --------- Cartesian 模式参数 ---------
	{
		auto p = ADD_MODEL_ARRAY_PARAM(Position_Initial_XYZ, Position_Initial_XYZSize);
		p.SetDefaultValue("[0 20e3 5e3]");
		p.SetUnit(SystemVueModelBuilder::Units::LENGTH);
		p.SetDescription("The initial range in antenna boresight direction");
		p.SetHideCondition("Coordinate_Mode ~= 1");
	}
	{
		auto p = ADD_MODEL_ARRAY_PARAM(Velocity_Initial_XYZ, Velocity_Initial_XYZSize);
		p.SetDefaultValue("[0 0 0]");
		p.SetDescription("The initial velocity of the target");
		p.SetHideCondition("Coordinate_Mode ~= 1");
	}
	{
		auto p = ADD_MODEL_ARRAY_PARAM(Accelerate_XYZ, Accelerate_XYZSize);
		p.SetDefaultValue("[0 0 0]");
		p.SetDescription("The accelerate of the target");
		p.SetHideCondition("Coordinate_Mode ~= 1");
	}
	{
		auto p = ADD_MODEL_ARRAY_PARAM(Jerk_XYZ, Jerk_XYZSize);
		p.SetDefaultValue("[0 0 0]");
		p.SetDescription("The jerk of the target");
		p.SetHideCondition("Coordinate_Mode ~= 1");
	}

	// --------- 公共参数 ---------
	{
		auto p = ADD_MODEL_PARAM(TimeStep);
		p.SetDefaultValue("1e-9");
		p.SetUnit(SystemVueModelBuilder::Units::TIME);
		p.SetDescription("The time step value");
	}

	return true;
}
#endif // SV_CODE_GEN

RADAR_TargetTrajectory::RADAR_TargetTrajectory(): Coordinate_Mode(Spherical), 
Range_Initial(20e3), 
ElevationAngle(0.0), 
AzimuthAngle(0.0), 
Velocity_Initial(0.0), 
Accelerate(0.0), 
Jerk(0.0), 
Position_Initial_XYZ(nullptr), 
Position_Initial_XYZSize(0), 
Velocity_Initial_XYZ(nullptr), 
Velocity_Initial_XYZSize(0), 
Accelerate_XYZ(nullptr), 
Accelerate_XYZSize(0), 
Jerk_XYZ(nullptr), 
Jerk_XYZSize(0), 
TimeStep(1e-9), 
sampleIndex_(0ULL)
{
}

double RADAR_TargetTrajectory::get_array_value_(const double* p, int size, int idx, double defval)
{
	if (p == nullptr || idx < 0 || idx >= size)
		return defval;
	return p[idx];
}

double RADAR_TargetTrajectory::calc_az_(double x, double y)
{
	// 黑盒对齐行为：
	// Az = atan(X/Y)，不是 atan2(X,Y)。
	// X=Y=0 时内置模型返回 Az=0。
	if (x == 0.0 && y == 0.0)
		return 0.0;

	if (y == 0.0)
		return (x > 0.0) ? (0.5 * kPi) : (-0.5 * kPi);

	return std::atan(x / y);
}

double RADAR_TargetTrajectory::calc_el_(double x, double y, double z)
{
	// 黑盒对齐行为：
	// El = atan(Z / sqrt(X^2+Y^2))。
	// 原点处输出 NaN；X=Y=0 且 Z!=0 时输出 +/-pi/2。
	const double horizontal = std::sqrt(x * x + y * y);

	if (horizontal == 0.0)
	{
		if (z > 0.0) return 0.5 * kPi;
		if (z < 0.0) return -0.5 * kPi;
		return std::numeric_limits<double>::quiet_NaN();
	}

	return std::atan(z / horizontal);
}

bool RADAR_TargetTrajectory::Setup()
{
	sampleIndex_ = 0ULL;

	// 内置帮助说明：每次 firing 只产生一个输出 token。
	Target_Pos_Z.SetRate(1);
	Target_Pos_Y.SetRate(1);
	Target_Pos_X.SetRate(1);
	Target_Az_Angle.SetRate(1);
	Target_El_Angle.SetRate(1);
	Delay_Output.SetRate(1);
	Range_Output.SetRate(1);

	return true;
}

bool RADAR_TargetTrajectory::Run()
{
	const double n = static_cast<double>(sampleIndex_);
	const double t = n * TimeStep;
	const double t2 = t * t;
	const double t3 = t2 * t;

	if (Coordinate_Mode == Spherical)
	{
		const double range = Range_Initial
			- Velocity_Initial * t
			- 0.5 * Accelerate * t2
			- (Jerk * t3 / 3.0);

		Range_Output[0] = range;
		Delay_Output[0] = 2.0 * range / kLightSpeed;

		// 注意：参数接口设置了 Units::ANGLE 后，SystemVue 会把界面输入的角度
		// 按角度单位传入模型。实测模型内部拿到的 AzimuthAngle / ElevationAngle
		// 已经是弧度值，因此这里必须直接输出，不能再次执行 deg->rad 转换。
		Target_Az_Angle[0] = AzimuthAngle;
		Target_El_Angle[0] = ElevationAngle;

		// 黑盒结果：Spherical 模式下 X/Y/Z 不是有效坐标输出，
		// 而是稳定的 1e-300 量级残留值；因此这里给出确定性的极小值。
		Target_Pos_X[0] = kSphericalInvalidXYZ;
		Target_Pos_Y[0] = kSphericalInvalidXYZ;
		Target_Pos_Z[0] = kSphericalInvalidXYZ;
	}
	else // Cartesian
	{
		const double x0 = get_array_value_(Position_Initial_XYZ, Position_Initial_XYZSize, 0, 0.0);
		const double y0 = get_array_value_(Position_Initial_XYZ, Position_Initial_XYZSize, 1, 20e3);
		const double z0 = get_array_value_(Position_Initial_XYZ, Position_Initial_XYZSize, 2, 5e3);

		const double vx = get_array_value_(Velocity_Initial_XYZ, Velocity_Initial_XYZSize, 0, 0.0);
		const double vy = get_array_value_(Velocity_Initial_XYZ, Velocity_Initial_XYZSize, 1, 0.0);
		const double vz = get_array_value_(Velocity_Initial_XYZ, Velocity_Initial_XYZSize, 2, 0.0);

		const double ax = get_array_value_(Accelerate_XYZ, Accelerate_XYZSize, 0, 0.0);
		const double ay = get_array_value_(Accelerate_XYZ, Accelerate_XYZSize, 1, 0.0);
		const double az = get_array_value_(Accelerate_XYZ, Accelerate_XYZSize, 2, 0.0);

		const double jx = get_array_value_(Jerk_XYZ, Jerk_XYZSize, 0, 0.0);
		const double jy = get_array_value_(Jerk_XYZ, Jerk_XYZSize, 1, 0.0);
		const double jz = get_array_value_(Jerk_XYZ, Jerk_XYZSize, 2, 0.0);

		const double x = x0 - vx * t - 0.5 * ax * t2 - (jx * t3 / 3.0);
		const double y = y0 - vy * t - 0.5 * ay * t2 - (jy * t3 / 3.0);
		const double z = z0 - vz * t - 0.5 * az * t2 - (jz * t3 / 3.0);

		const double range = std::sqrt(x * x + y * y + z * z);

		Target_Pos_X[0] = x;
		Target_Pos_Y[0] = y;
		Target_Pos_Z[0] = z;

		Range_Output[0] = range;
		Delay_Output[0] = 2.0 * range / kLightSpeed;

		Target_Az_Angle[0] = calc_az_(x, y);
		Target_El_Angle[0] = calc_el_(x, y, z);
	}

	++sampleIndex_;
	return true;
}
