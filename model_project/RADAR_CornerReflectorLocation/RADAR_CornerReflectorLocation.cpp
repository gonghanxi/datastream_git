#include "RADAR_CornerReflectorLocation.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

const double RADAR_CornerReflectorLocation::kPi = 3.1415926535897932384626433832795;
const double RADAR_CornerReflectorLocation::kDegToRad = RADAR_CornerReflectorLocation::kPi / 180.0;
const double RADAR_CornerReflectorLocation::kRadToDeg = 180.0 / RADAR_CornerReflectorLocation::kPi;
const double RADAR_CornerReflectorLocation::kSpeedOfLight = 299792458.0;

// 与常见 WGS84 椭球参数保持一致的近似值。
// ECI 部分为简化实现：先 LLA->ECEF，再按地球自转角绕 Z 轴近似转到 ECI。
const double RADAR_CornerReflectorLocation::kEarthSemiMajorAxis = 6378137.0;
const double RADAR_CornerReflectorLocation::kEarthSemiMinorAxis = 6356752.0;
const double RADAR_CornerReflectorLocation::kEarthRotationRate = 7.2921151467e-5;

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(RADAR_CornerReflectorLocation)
{
	SET_MODEL_DESCRIPTION("Radar Corner Reflector Location and Physical RCS");
	SET_MODEL_CATEGORY("Environments");

	// --------- 输入端口 ---------
	{
		auto p = ADD_MODEL_INPUT(Roll);
		p.SetDescription("The roll angle of corner reflector body frame (deg)");
		p.SetOptional(true);
	}
	{
		auto p = ADD_MODEL_INPUT(Pitch);
		p.SetDescription("The pitch angle of corner reflector body frame (deg)");
		p.SetOptional(true);
	}
	{
		auto p = ADD_MODEL_INPUT(Yaw);
		p.SetDescription("The yaw angle of corner reflector body frame (deg)");
		p.SetOptional(true);
	}
	{
		auto p = ADD_MODEL_INPUT(RadarX);
		p.SetDescription("Radar X position in the same coordinate frame as Pos (m)");
		p.SetOptional(true);
	}
	{
		auto p = ADD_MODEL_INPUT(RadarY);
		p.SetDescription("Radar Y position in the same coordinate frame as Pos (m)");
		p.SetOptional(true);
	}
	{
		auto p = ADD_MODEL_INPUT(RadarZ);
		p.SetDescription("Radar Z position in the same coordinate frame as Pos (m)");
		p.SetOptional(true);
	}
	{
		auto p = ADD_MODEL_INPUT(CarrierFreqIn);
		p.SetDescription("Carrier frequency input (Hz). If disconnected, CarrierFreq parameter is used.");
		p.SetOptional(true);
	}

	// --------- 输出端口 ---------
	{
		auto p = ADD_MODEL_OUTPUT(Pos);
		p.SetDescription("Corner reflector equivalent position. Each bus channel is a 3x1 real matrix [X;Y;Z].");
	}
	{
		auto p = ADD_MODEL_OUTPUT(CornerRCS);
		p.SetDescription("Corner reflector RCS. Unit is m^2 if Linear_m2, or dBsm if dBsm.");
	}

	// --------- Trajectory_Mode ---------
	{
		auto p = ADD_MODEL_ENUM_PARAM(Trajectory_Mode, Trajectory_ModeEnum);
		p.AddEnumeration("ECI_Frame", RADAR_CornerReflectorLocation::ECI_Frame);
		p.AddEnumeration("UserDefined", RADAR_CornerReflectorLocation::User_Defined);
		p.AddEnumeration("SimpleXYZ_Frame", RADAR_CornerReflectorLocation::SimpleXYZ_Frame);
		p.SetDefaultValue("SimpleXYZ_Frame");
		p.SetDescription("Trajectory generation mode: ECI_Frame, UserDefined, SimpleXYZ_Frame");
	}

	// --------- Motion_Mode ---------
	{
		auto p = ADD_MODEL_ENUM_PARAM(Motion_Mode, Motion_ModeEnum);
		p.AddEnumeration("Fixed_Mode", RADAR_CornerReflectorLocation::Fixed_Mode);
		p.AddEnumeration("Moving_Mode", RADAR_CornerReflectorLocation::Moving_Mode);
		p.SetDefaultValue("Fixed_Mode");
		p.SetDescription("Corner reflector motion mode. Fixed_Mode keeps centroid fixed. Moving_Mode enables velocity, acceleration and jerk parameters.");
		p.SetHideCondition("Trajectory_Mode == 1");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(FileName);
		p.SetDefaultValue("");
		p.SetDescription("UserDefined file path. Each line format: x0 y0 z0 rcs0 x1 y1 z1 rcs1 ...");
		p.SetParamAsFile();
		p.SetHideCondition("Trajectory_Mode ~= 1");
	}

	{
		auto p = ADD_MODEL_PARAM(NumberOfCornerReflector);
		p.SetDefaultValue("1");
		p.SetDescription("The number of corner reflectors. It is also the expected bus width of Pos and CornerRCS.");
	}

	// --------- 角反射器类型 ---------
	{
		auto p = ADD_MODEL_ENUM_PARAM(ReflectorType, ReflectorTypeEnum);
		p.AddEnumeration("Triangular_Trihedral", RADAR_CornerReflectorLocation::Triangular_Trihedral);
		p.AddEnumeration("Square_Trihedral", RADAR_CornerReflectorLocation::Square_Trihedral);
		p.SetDefaultValue("Triangular_Trihedral");
		p.SetDescription("Corner reflector type. Triangular trihedral: sigma=4*pi*a^4/(3*lambda^2). Square trihedral: sigma=12*pi*a^4/lambda^2.");
		p.SetHideCondition("Trajectory_Mode == 1");
	}

	{
		auto p = ADD_MODEL_ENUM_PARAM(RCS_Model, RCSModelEnum);
		p.AddEnumeration("PeakOnly", RADAR_CornerReflectorLocation::PeakOnly);
		p.AddEnumeration("BoresightCone", RADAR_CornerReflectorLocation::BoresightCone);
		p.SetDefaultValue("PeakOnly");
		p.SetDescription("RCS model. PeakOnly uses boresight peak RCS. BoresightCone additionally checks aperture direction and boresight half-angle.");
		p.SetHideCondition("Trajectory_Mode == 1");
	}

	{
		auto p = ADD_MODEL_ENUM_PARAM(RCS_OutputUnit, RCSOutputUnitEnum);
		p.AddEnumeration("Linear_m2", RADAR_CornerReflectorLocation::Linear_m2);
		p.AddEnumeration("dBsm", RADAR_CornerReflectorLocation::dBsm);
		p.SetDefaultValue("Linear_m2");
		p.SetDescription("RCS output unit: linear square meters or dBsm.");
	}

	{
		auto p = ADD_MODEL_ARRAY_PARAM(CornerLoc, CornerLocSize);
		p.SetDefaultValue("[0 0 0]");
		p.SetUnit(SystemVueModelBuilder::Units::LENGTH);
		p.SetDescription("Corner reflector location offset relative to model centroid, in body frame. Format: [x0 y0 z0 x1 y1 z1 ...].");
		p.SetHideCondition("Trajectory_Mode == 1");
	}

	{
		auto p = ADD_MODEL_ARRAY_PARAM(EdgeLength, EdgeLengthSize);
		p.SetDefaultValue("[1]");
		p.SetUnit(SystemVueModelBuilder::Units::LENGTH);
		p.SetDescription("Interior edge length a of trihedral corner reflector. Length can be 1 for broadcasting or N for each reflector.");
		p.SetHideCondition("Trajectory_Mode == 1");
	}

	{
		auto p = ADD_MODEL_ARRAY_PARAM(Efficiency, EfficiencySize);
		p.SetDefaultValue("[1]");
		p.SetDescription("RCS efficiency coefficient. sigma_out = Efficiency * sigma_ideal. Length can be 1 or N.");
		p.SetHideCondition("Trajectory_Mode == 1");
	}

	{
		auto p = ADD_MODEL_ARRAY_PARAM(CornerRollOffset, CornerRollOffsetSize);
		p.SetDefaultValue("[0]");
		p.SetDescription("Per-corner roll offset in deg. Length can be 1 or N.");
		p.SetHideCondition("Trajectory_Mode == 1");
	}
	{
		auto p = ADD_MODEL_ARRAY_PARAM(CornerPitchOffset, CornerPitchOffsetSize);
		p.SetDefaultValue("[0]");
		p.SetDescription("Per-corner pitch offset in deg. Length can be 1 or N.");
		p.SetHideCondition("Trajectory_Mode == 1");
	}
	{
		auto p = ADD_MODEL_ARRAY_PARAM(CornerYawOffset, CornerYawOffsetSize);
		p.SetDefaultValue("[0]");
		p.SetDescription("Per-corner yaw offset in deg. Length can be 1 or N.");
		p.SetHideCondition("Trajectory_Mode == 1");
	}

	{
		auto p = ADD_MODEL_ARRAY_PARAM(PhaseCenterOffset, PhaseCenterOffsetSize);
		p.SetDefaultValue("[0 0 0]");
		p.SetUnit(SystemVueModelBuilder::Units::LENGTH);
		p.SetDescription("Equivalent phase-center offset in body frame. Default [0 0 0]. Format can be 3 elements or 3*N elements.");
		p.SetHideCondition("Trajectory_Mode == 1");
	}

	// --------- RCS 计算参数 ---------
	{
		auto p = ADD_MODEL_PARAM(CarrierFreq);
		p.SetDefaultValue("10e9");
		p.SetDescription("Carrier frequency in Hz. Wavelength lambda = c / CarrierFreq.");
		p.SetHideCondition("Trajectory_Mode == 1");
	}
	{
		auto p = ADD_MODEL_PARAM(BoresightHalfAngle);
		p.SetDefaultValue("30");
		p.SetDescription("Boresight half-angle in deg. Used only when RCS_Model is BoresightCone.");
		p.SetHideCondition("Trajectory_Mode == 1 || RCS_Model ~= 1");
	}
	{
		auto p = ADD_MODEL_PARAM(RCS_Floor);
		p.SetDefaultValue("0");
		p.SetDescription("Minimum linear RCS in m^2 when reflector is not illuminated by aperture or outside boresight cone.");
		p.SetHideCondition("Trajectory_Mode == 1 || RCS_Model ~= 1");
	}
	{
		auto p = ADD_MODEL_ARRAY_PARAM(Radar_Position_XYZ, Radar_Position_XYZSize);
		p.SetDefaultValue("[0 0 0]");
		p.SetUnit(SystemVueModelBuilder::Units::LENGTH);
		p.SetDescription("Default radar position if RadarX/RadarY/RadarZ input ports are disconnected.");
		p.SetHideCondition("Trajectory_Mode == 1 || RCS_Model ~= 1");
	}

	// --------- ECI_Frame 参数 ---------
	{
		auto p = ADD_MODEL_ARRAY_PARAM(Position_Initial, Position_InitialSize);
		p.SetDefaultValue("[0 0 0]");
		p.SetDescription("Initial position in LLA frame: [longitude(deg), latitude(deg), height(m)].");
		p.SetHideCondition("Trajectory_Mode ~= 0");
	}
	{
		auto p = ADD_MODEL_PARAM(Velocity_Initial);
		p.SetDefaultValue("0");
		p.SetDescription("Initial velocity of the target centroid along yaw/pitch direction, unit m/s. Visible only in Moving_Mode.");
		p.SetHideCondition("Trajectory_Mode ~= 0 || Motion_Mode ~= 1");
	}
	{
		auto p = ADD_MODEL_PARAM(Accelerate_Initial);
		p.SetDefaultValue("0");
		p.SetDescription("Initial acceleration along yaw/pitch direction, unit m/s^2. Visible only in Moving_Mode.");
		p.SetHideCondition("Trajectory_Mode ~= 0 || Motion_Mode ~= 1");
	}

	// --------- SimpleXYZ_Frame 参数 ---------
	{
		auto p = ADD_MODEL_ARRAY_PARAM(Position_Initial_XYZ, Position_Initial_XYZSize);
		p.SetDefaultValue("[0 0 0]");
		p.SetUnit(SystemVueModelBuilder::Units::LENGTH);
		p.SetDescription("Initial centroid position in Cartesian coordinate system.");
		p.SetHideCondition("Trajectory_Mode ~= 2");
	}
	{
		auto p = ADD_MODEL_ARRAY_PARAM(Velocity_Initial_XYZ, Velocity_Initial_XYZSize);
		p.SetDefaultValue("[0 0 0]");
		p.SetDescription("Initial centroid velocity in Cartesian coordinate system. Visible only in Moving_Mode.");
		p.SetHideCondition("Trajectory_Mode ~= 2 || Motion_Mode ~= 1");
	}
	{
		auto p = ADD_MODEL_ARRAY_PARAM(Accelerate_XYZ, Accelerate_XYZSize);
		p.SetDefaultValue("[0 0 0]");
		p.SetDescription("Centroid acceleration in Cartesian coordinate system. Visible only in Moving_Mode.");
		p.SetHideCondition("Trajectory_Mode ~= 2 || Motion_Mode ~= 1");
	}
	{
		auto p = ADD_MODEL_ARRAY_PARAM(Jerk_XYZ, Jerk_XYZSize);
		p.SetDefaultValue("[0 0 0]");
		p.SetDescription("Centroid jerk in Cartesian coordinate system. Visible only in Moving_Mode.");
		p.SetHideCondition("Trajectory_Mode ~= 2 || Motion_Mode ~= 1");
	}

	{
		auto p = ADD_MODEL_PARAM(TimeStep);
		p.SetDefaultValue("1e-9");
		p.SetUnit(SystemVueModelBuilder::Units::TIME);
		p.SetDescription("Time step value used by internal trajectory generator.");
	}

	return true;
}
#endif // SV_CODE_GEN

RADAR_CornerReflectorLocation::RADAR_CornerReflectorLocation()
	: Trajectory_Mode(SimpleXYZ_Frame)
	, Motion_Mode(Fixed_Mode)
	, NumberOfCornerReflector(1)
	, FileName(nullptr)
	, ReflectorType(Triangular_Trihedral)
	, RCS_Model(PeakOnly)
	, RCS_OutputUnit(Linear_m2)
	, CornerLoc(nullptr)
	, CornerLocSize(0)
	, EdgeLength(nullptr)
	, EdgeLengthSize(0)
	, Efficiency(nullptr)
	, EfficiencySize(0)
	, CornerRollOffset(nullptr)
	, CornerRollOffsetSize(0)
	, CornerPitchOffset(nullptr)
	, CornerPitchOffsetSize(0)
	, CornerYawOffset(nullptr)
	, CornerYawOffsetSize(0)
	, PhaseCenterOffset(nullptr)
	, PhaseCenterOffsetSize(0)
	, CarrierFreq(10e9)
	, BoresightHalfAngle(30.0)
	, RCS_Floor(0.0)
	, Radar_Position_XYZ(nullptr)
	, Radar_Position_XYZSize(0)
	, Position_Initial(nullptr)
	, Position_InitialSize(0)
	, Velocity_Initial(0.0)
	, Accelerate_Initial(0.0)
	, Position_Initial_XYZ(nullptr)
	, Position_Initial_XYZSize(0)
	, Velocity_Initial_XYZ(nullptr)
	, Velocity_Initial_XYZSize(0)
	, Accelerate_XYZ(nullptr)
	, Accelerate_XYZSize(0)
	, Jerk_XYZ(nullptr)
	, Jerk_XYZSize(0)
	, TimeStep(1e-9)
	, sampleIndex_(0ULL)
	, p0Ecef_(make_vec_(0.0, 0.0, 0.0))
	, eastEcef_(make_vec_(0.0, 1.0, 0.0))
	, northEcef_(make_vec_(0.0, 0.0, 1.0))
	, upEcef_(make_vec_(1.0, 0.0, 0.0))
	, lonRad_(0.0)
	, latRad_(0.0)
	, userPathIndex_(0U)
{
	init_empty_user_sample_();
}

double RADAR_CornerReflectorLocation::get_array_value_(const double* p, int size, int idx, double defval)
{
	if (p == nullptr || idx < 0 || idx >= size)
		return defval;
	return p[idx];
}

double RADAR_CornerReflectorLocation::get_array_broadcast_(const double* p, int size, int idx, double defval)
{
	if (p == nullptr || size <= 0)
		return defval;

	if (size == 1)
		return p[0];

	if (idx >= 0 && idx < size)
		return p[idx];

	return defval;
}

RADAR_CornerReflectorLocation::Vec3 RADAR_CornerReflectorLocation::make_vec_(double x, double y, double z)
{
	Vec3 v;
	v.x = x;
	v.y = y;
	v.z = z;
	return v;
}

RADAR_CornerReflectorLocation::Vec3 RADAR_CornerReflectorLocation::add_(const Vec3& a, const Vec3& b)
{
	return make_vec_(a.x + b.x, a.y + b.y, a.z + b.z);
}

RADAR_CornerReflectorLocation::Vec3 RADAR_CornerReflectorLocation::sub_(const Vec3& a, const Vec3& b)
{
	return make_vec_(a.x - b.x, a.y - b.y, a.z - b.z);
}

RADAR_CornerReflectorLocation::Vec3 RADAR_CornerReflectorLocation::scale_(const Vec3& a, double s)
{
	return make_vec_(a.x * s, a.y * s, a.z * s);
}

double RADAR_CornerReflectorLocation::dot_(const Vec3& a, const Vec3& b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

double RADAR_CornerReflectorLocation::norm_(const Vec3& a)
{
	return std::sqrt(dot_(a, a));
}

RADAR_CornerReflectorLocation::Vec3 RADAR_CornerReflectorLocation::normalize_(const Vec3& a, const Vec3& fallback)
{
	const double n = norm_(a);
	if (n <= 0.0)
		return fallback;
	return scale_(a, 1.0 / n);
}

RADAR_CornerReflectorLocation::Mat3 RADAR_CornerReflectorLocation::identity_mat_()
{
	Mat3 A;
	A.m[0][0] = 1.0; A.m[0][1] = 0.0; A.m[0][2] = 0.0;
	A.m[1][0] = 0.0; A.m[1][1] = 1.0; A.m[1][2] = 0.0;
	A.m[2][0] = 0.0; A.m[2][1] = 0.0; A.m[2][2] = 1.0;
	return A;
}

RADAR_CornerReflectorLocation::Mat3 RADAR_CornerReflectorLocation::mat_mul_(const Mat3& A, const Mat3& B)
{
	Mat3 C;
	for (int r = 0; r < 3; ++r)
	{
		for (int c = 0; c < 3; ++c)
		{
			C.m[r][c] =
				A.m[r][0] * B.m[0][c] +
				A.m[r][1] * B.m[1][c] +
				A.m[r][2] * B.m[2][c];
		}
	}
	return C;
}

RADAR_CornerReflectorLocation::Vec3 RADAR_CornerReflectorLocation::mat_vec_(const Mat3& A, const Vec3& v)
{
	return make_vec_(
		A.m[0][0] * v.x + A.m[0][1] * v.y + A.m[0][2] * v.z,
		A.m[1][0] * v.x + A.m[1][1] * v.y + A.m[1][2] * v.z,
		A.m[2][0] * v.x + A.m[2][1] * v.y + A.m[2][2] * v.z
	);
}

RADAR_CornerReflectorLocation::Mat3 RADAR_CornerReflectorLocation::transpose_(const Mat3& A)
{
	Mat3 B;
	for (int r = 0; r < 3; ++r)
	{
		for (int c = 0; c < 3; ++c)
		{
			B.m[r][c] = A.m[c][r];
		}
	}
	return B;
}

RADAR_CornerReflectorLocation::Mat3 RADAR_CornerReflectorLocation::rot_x_(double a)
{
	const double c = std::cos(a);
	const double s = std::sin(a);

	Mat3 R = identity_mat_();
	R.m[1][1] = c;  R.m[1][2] = -s;
	R.m[2][1] = s;  R.m[2][2] = c;
	return R;
}

RADAR_CornerReflectorLocation::Mat3 RADAR_CornerReflectorLocation::rot_y_(double a)
{
	const double c = std::cos(a);
	const double s = std::sin(a);

	Mat3 R = identity_mat_();
	R.m[0][0] = c;  R.m[0][2] = s;
	R.m[2][0] = -s; R.m[2][2] = c;
	return R;
}

RADAR_CornerReflectorLocation::Mat3 RADAR_CornerReflectorLocation::rot_z_(double a)
{
	const double c = std::cos(a);
	const double s = std::sin(a);

	Mat3 R = identity_mat_();
	R.m[0][0] = c;  R.m[0][1] = -s;
	R.m[1][0] = s;  R.m[1][1] = c;
	return R;
}

RADAR_CornerReflectorLocation::Mat3 RADAR_CornerReflectorLocation::rpy_matrix_(double rollRad, double pitchRad, double yawRad)
{
	// 常见航空/雷达工程姿态顺序：
	//   R = Rz(yaw) * Ry(pitch) * Rx(roll)
	// 这里使用列向量主动旋转：v_global = R * v_body。
	return mat_mul_(mat_mul_(rot_z_(yawRad), rot_y_(pitchRad)), rot_x_(rollRad));
}

RADAR_CornerReflectorLocation::Vec3 RADAR_CornerReflectorLocation::lla_to_ecef_(double lonRad, double latRad, double h)
{
	const double a = kEarthSemiMajorAxis;
	const double b = kEarthSemiMinorAxis;

	const double a2 = a * a;
	const double b2 = b * b;
	const double e2 = 1.0 - b2 / a2;

	const double sinLat = std::sin(latRad);
	const double cosLat = std::cos(latRad);
	const double sinLon = std::sin(lonRad);
	const double cosLon = std::cos(lonRad);

	const double N = a / std::sqrt(1.0 - e2 * sinLat * sinLat);

	const double x = (N + h) * cosLat * cosLon;
	const double y = (N + h) * cosLat * sinLon;
	const double z = (N * (1.0 - e2) + h) * sinLat;

	return make_vec_(x, y, z);
}

RADAR_CornerReflectorLocation::Mat3 RADAR_CornerReflectorLocation::ned_to_ecef_matrix_(const Vec3& north, const Vec3& east, const Vec3& up)
{
	// NED -> ECEF：
	//   local x = North
	//   local y = East
	//   local z = Down = -Up
	// 矩阵的三列分别为 N、E、D 在 ECEF 中的表示。
	Mat3 M;
	M.m[0][0] = north.x; M.m[0][1] = east.x; M.m[0][2] = -up.x;
	M.m[1][0] = north.y; M.m[1][1] = east.y; M.m[1][2] = -up.y;
	M.m[2][0] = north.z; M.m[2][1] = east.z; M.m[2][2] = -up.z;
	return M;
}

RADAR_CornerReflectorLocation::Mat3 RADAR_CornerReflectorLocation::ecef_to_eci_matrix_(double theta)
{
	// 简化 ECEF->ECI：绕 Z 轴旋转地球自转角 theta。
	return rot_z_(theta);
}

double RADAR_CornerReflectorLocation::safe_log10_(double x)
{
	const double eps = 1.0e-300;
	return std::log10((x > eps) ? x : eps);
}

double RADAR_CornerReflectorLocation::clamp_(double x, double lo, double hi)
{
	return std::max(lo, std::min(hi, x));
}

bool RADAR_CornerReflectorLocation::validate_params_() const
{
	const int n = (NumberOfCornerReflector > 0) ? NumberOfCornerReflector : 1;

	if (Trajectory_Mode == User_Defined)
		return true;

	if (CornerLocSize != 3 * n)
	{
		std::cerr << "RADAR_CornerReflectorLocation: CornerLoc should have 3*NumberOfCornerReflector elements." << std::endl;
		return false;
	}

	if (!(EdgeLengthSize == 1 || EdgeLengthSize == n))
	{
		std::cerr << "RADAR_CornerReflectorLocation: EdgeLength size should be 1 or NumberOfCornerReflector." << std::endl;
		return false;
	}

	if (!(EfficiencySize == 0 || EfficiencySize == 1 || EfficiencySize == n))
	{
		std::cerr << "RADAR_CornerReflectorLocation: Efficiency size should be 1 or NumberOfCornerReflector." << std::endl;
		return false;
	}

	if (!(CornerRollOffsetSize == 0 || CornerRollOffsetSize == 1 || CornerRollOffsetSize == n))
	{
		std::cerr << "RADAR_CornerReflectorLocation: CornerRollOffset size should be 1 or NumberOfCornerReflector." << std::endl;
		return false;
	}

	if (!(CornerPitchOffsetSize == 0 || CornerPitchOffsetSize == 1 || CornerPitchOffsetSize == n))
	{
		std::cerr << "RADAR_CornerReflectorLocation: CornerPitchOffset size should be 1 or NumberOfCornerReflector." << std::endl;
		return false;
	}

	if (!(CornerYawOffsetSize == 0 || CornerYawOffsetSize == 1 || CornerYawOffsetSize == n))
	{
		std::cerr << "RADAR_CornerReflectorLocation: CornerYawOffset size should be 1 or NumberOfCornerReflector." << std::endl;
		return false;
	}

	if (!(PhaseCenterOffsetSize == 0 || PhaseCenterOffsetSize == 3 || PhaseCenterOffsetSize == 3 * n))
	{
		std::cerr << "RADAR_CornerReflectorLocation: PhaseCenterOffset size should be 3 or 3*NumberOfCornerReflector." << std::endl;
		return false;
	}

	if (CarrierFreq <= 0.0)
	{
		std::cerr << "RADAR_CornerReflectorLocation: CarrierFreq should be positive." << std::endl;
		return false;
	}

	return true;
}

void RADAR_CornerReflectorLocation::init_empty_user_sample_()
{
	const int n = (NumberOfCornerReflector > 0) ? NumberOfCornerReflector : 1;
	lastUserSample_.pos.assign(static_cast<std::size_t>(n), make_vec_(0.0, 0.0, 0.0));
	lastUserSample_.rcs.assign(static_cast<std::size_t>(n), 0.0);
}

bool RADAR_CornerReflectorLocation::load_user_file_()
{
	userPath_.clear();
	userPathIndex_ = 0U;
	init_empty_user_sample_();

	const int n = (NumberOfCornerReflector > 0) ? NumberOfCornerReflector : 1;

	if (FileName == nullptr || FileName[0] == '\0')
		return true;

	std::ifstream fin(FileName);
	if (!fin)
		return true;

	std::string line;
	while (std::getline(fin, line))
	{
		std::istringstream iss(line);
		UserSample sample;
		sample.pos.reserve(static_cast<std::size_t>(n));
		sample.rcs.reserve(static_cast<std::size_t>(n));

		bool ok = true;
		for (int i = 0; i < n; ++i)
		{
			double x = 0.0;
			double y = 0.0;
			double z = 0.0;
			double r = 0.0;
			if (!(iss >> x >> y >> z >> r))
			{
				ok = false;
				break;
			}

			sample.pos.push_back(make_vec_(x, y, z));
			sample.rcs.push_back(r);
		}

		if (ok && static_cast<int>(sample.pos.size()) == n)
			userPath_.push_back(sample);
	}

	if (!userPath_.empty())
		lastUserSample_ = userPath_.front();

	return true;
}

RADAR_CornerReflectorLocation::Vec3 RADAR_CornerReflectorLocation::get_corner_loc_(int idx) const
{
	return make_vec_(
		get_array_value_(CornerLoc, CornerLocSize, 3 * idx + 0, 0.0),
		get_array_value_(CornerLoc, CornerLocSize, 3 * idx + 1, 0.0),
		get_array_value_(CornerLoc, CornerLocSize, 3 * idx + 2, 0.0)
	);
}

RADAR_CornerReflectorLocation::Vec3 RADAR_CornerReflectorLocation::get_phase_center_offset_(int idx) const
{
	if (PhaseCenterOffset == nullptr || PhaseCenterOffsetSize <= 0)
		return make_vec_(0.0, 0.0, 0.0);

	if (PhaseCenterOffsetSize == 3)
	{
		return make_vec_(
			PhaseCenterOffset[0],
			PhaseCenterOffset[1],
			PhaseCenterOffset[2]
		);
	}

	return make_vec_(
		get_array_value_(PhaseCenterOffset, PhaseCenterOffsetSize, 3 * idx + 0, 0.0),
		get_array_value_(PhaseCenterOffset, PhaseCenterOffsetSize, 3 * idx + 1, 0.0),
		get_array_value_(PhaseCenterOffset, PhaseCenterOffsetSize, 3 * idx + 2, 0.0)
	);
}

double RADAR_CornerReflectorLocation::get_edge_length_(int idx) const
{
	return get_array_broadcast_(EdgeLength, EdgeLengthSize, idx, 1.0);
}

double RADAR_CornerReflectorLocation::get_efficiency_(int idx) const
{
	double eta = get_array_broadcast_(Efficiency, EfficiencySize, idx, 1.0);
	if (eta < 0.0)
		eta = 0.0;
	return eta;
}

double RADAR_CornerReflectorLocation::get_carrier_freq_()
{
	// 若端口连接并且提供正频率，则优先使用输入端口。
	// 注意：本函数读取 CarrierFreqIn[0]，因此不能是 const 成员函数。
	if (CarrierFreqIn.GetSize() > 0)
	{
		const double f = CarrierFreqIn[0];
		if (f > 0.0)
			return f;
	}

	return CarrierFreq;
}

RADAR_CornerReflectorLocation::Vec3 RADAR_CornerReflectorLocation::get_radar_position_()
{
	const double px = get_array_value_(Radar_Position_XYZ, Radar_Position_XYZSize, 0, 0.0);
	const double py = get_array_value_(Radar_Position_XYZ, Radar_Position_XYZSize, 1, 0.0);
	const double pz = get_array_value_(Radar_Position_XYZ, Radar_Position_XYZSize, 2, 0.0);

	double x = px;
	double y = py;
	double z = pz;

	// 注意：本函数读取 RadarX/Y/Z[0]，因此不能是 const 成员函数。
	if (RadarX.GetSize() > 0) x = RadarX[0];
	if (RadarY.GetSize() > 0) y = RadarY[0];
	if (RadarZ.GetSize() > 0) z = RadarZ[0];

	return make_vec_(x, y, z);
}

double RADAR_CornerReflectorLocation::peak_rcs_linear_(int idx, double freqHz) const
{
	const double a = get_edge_length_(idx);
	if (a <= 0.0 || freqHz <= 0.0)
		return 0.0;

	const double lambda = kSpeedOfLight / freqHz;
	const double a2 = a * a;
	const double a4 = a2 * a2;

	double sigma = 0.0;

	if (ReflectorType == Triangular_Trihedral)
	{
		// 三角形三面角反射器峰值 RCS：
		//   sigma = 4*pi*a^4 / (3*lambda^2)
		// 其中 a 是三面角内部边长。
		sigma = 4.0 * kPi * a4 / (3.0 * lambda * lambda);
	}
	else
	{
		// 正方形三面角反射器峰值 RCS：
		//   sigma = 12*pi*a^4 / lambda^2
		// 同样假设金属面理想、尺寸远大于波长，并且雷达正对 boresight。
		sigma = 12.0 * kPi * a4 / (lambda * lambda);
	}

	sigma *= get_efficiency_(idx);

	if (sigma < 0.0)
		sigma = 0.0;

	return sigma;
}

double RADAR_CornerReflectorLocation::calc_corner_rcs_(int idx,
	const Vec3& reflectorPos,
	const Mat3& bodyToGlobal,
	double freqHz)
{
	double sigma = peak_rcs_linear_(idx, freqHz);

	if (RCS_Model == BoresightCone)
	{
		const Vec3 radarPos = get_radar_position_();
		const Vec3 uGlobal = normalize_(sub_(radarPos, reflectorPos), make_vec_(1.0, 0.0, 0.0));

		// 转到角反射器 body frame。
		// body frame 中三块面为 x=0,y=0,z=0，开口位于 +x,+y,+z 象限。
		const Mat3 globalToBody = transpose_(bodyToGlobal);
		const Vec3 uBody = mat_vec_(globalToBody, uGlobal);

		const bool inAperture = (uBody.x > 0.0 && uBody.y > 0.0 && uBody.z > 0.0);

		const Vec3 boresight = normalize_(make_vec_(1.0, 1.0, 1.0), make_vec_(1.0, 0.0, 0.0));
		const Vec3 uBodyNorm = normalize_(uBody, boresight);

		const double cosAng = clamp_(dot_(uBodyNorm, boresight), -1.0, 1.0);
		const double aspectDeg = std::acos(cosAng) * kRadToDeg;

		if (!inAperture || aspectDeg > BoresightHalfAngle)
		{
			sigma = (RCS_Floor > 0.0) ? RCS_Floor : 0.0;
		}
	}

	if (RCS_OutputUnit == dBsm)
	{
		return 10.0 * safe_log10_(sigma);
	}

	return sigma;
}

void RADAR_CornerReflectorLocation::compute_simple_xyz_center_and_orientation_(Vec3& center, Mat3& bodyToGlobal)
{
	const double k = static_cast<double>(sampleIndex_);
	const double t = k * TimeStep;
	const double t2 = t * t;
	const double t3 = t2 * t;

	const double x0 = get_array_value_(Position_Initial_XYZ, Position_Initial_XYZSize, 0, 0.0);
	const double y0 = get_array_value_(Position_Initial_XYZ, Position_Initial_XYZSize, 1, 0.0);
	const double z0 = get_array_value_(Position_Initial_XYZ, Position_Initial_XYZSize, 2, 0.0);

	if (Motion_Mode == Moving_Mode)
	{
		const double vx = get_array_value_(Velocity_Initial_XYZ, Velocity_Initial_XYZSize, 0, 0.0);
		const double vy = get_array_value_(Velocity_Initial_XYZ, Velocity_Initial_XYZSize, 1, 0.0);
		const double vz = get_array_value_(Velocity_Initial_XYZ, Velocity_Initial_XYZSize, 2, 0.0);

		const double ax = get_array_value_(Accelerate_XYZ, Accelerate_XYZSize, 0, 0.0);
		const double ay = get_array_value_(Accelerate_XYZ, Accelerate_XYZSize, 1, 0.0);
		const double az = get_array_value_(Accelerate_XYZ, Accelerate_XYZSize, 2, 0.0);

		const double jx = get_array_value_(Jerk_XYZ, Jerk_XYZSize, 0, 0.0);
		const double jy = get_array_value_(Jerk_XYZ, Jerk_XYZSize, 1, 0.0);
		const double jz = get_array_value_(Jerk_XYZ, Jerk_XYZSize, 2, 0.0);

		// 运动模式：
		//   P = P0 + V*t + 1/2*A*t^2 + 1/6*J*t^3
		center = make_vec_(
			x0 + vx * t + 0.5 * ax * t2 + (jx * t3 / 6.0),
			y0 + vy * t + 0.5 * ay * t2 + (jy * t3 / 6.0),
			z0 + vz * t + 0.5 * az * t2 + (jz * t3 / 6.0)
		);
	}
	else
	{
		// 固定模式：
		// 只使用初始位置，不使用速度、加速度和 Jerk。
		center = make_vec_(x0, y0, z0);
	}

	// 注意：本函数读取 Roll/Pitch/Yaw[0]，因此不能是 const 成员函数。
	const double rollDeg = (Roll.GetSize() > 0) ? Roll[0] : 0.0;
	const double pitchDeg = (Pitch.GetSize() > 0) ? Pitch[0] : 0.0;
	const double yawDeg = (Yaw.GetSize() > 0) ? Yaw[0] : 0.0;

	bodyToGlobal = rpy_matrix_(
		rollDeg * kDegToRad,
		pitchDeg * kDegToRad,
		yawDeg * kDegToRad
	);
}

void RADAR_CornerReflectorLocation::compute_eci_center_and_orientation_(Vec3& center, Mat3& bodyToGlobal)
{
	// 注意：本函数读取 Roll/Pitch/Yaw[0]，因此不能是 const 成员函数。
	const double rollDeg = (Roll.GetSize() > 0) ? Roll[0] : 0.0;
	const double pitchDeg = (Pitch.GetSize() > 0) ? Pitch[0] : 0.0;
	const double yawDeg = (Yaw.GetSize() > 0) ? Yaw[0] : 0.0;

	const double roll = rollDeg * kDegToRad;
	const double pitch = pitchDeg * kDegToRad;
	const double yaw = yawDeg * kDegToRad;

	// ECI 模式下，姿态角同时用于：
	//   1) 指定质心运动方向；
	//   2) 指定角反射器 body frame 相对局部 NED 的姿态。
	//
	// 约定：
	//   yaw=0 指向 North；
	//   yaw=+90 指向 East；
	//   pitch=+90 指向 Up。
	const double dirEast = std::cos(pitch) * std::sin(yaw);
	const double dirNorth = std::cos(pitch) * std::cos(yaw);
	const double dirUp = std::sin(pitch);

	Vec3 dirEcef = add_(
		add_(scale_(eastEcef_, dirEast), scale_(northEcef_, dirNorth)),
		scale_(upEcef_, dirUp)
	);

	dirEcef = normalize_(dirEcef, northEcef_);

	const double t = static_cast<double>(sampleIndex_) * TimeStep;

	double distance = 0.0;
	if (Motion_Mode == Moving_Mode)
	{
		// 运动模式：
		// ECI 下沿 yaw/pitch 指定的局部方向运动。
		distance = Velocity_Initial * t + 0.5 * Accelerate_Initial * t * t;
	}
	else
	{
		// 固定模式：
		// 始终保持在 Position_Initial 对应的位置。
		distance = 0.0;
	}

	const Vec3 centerEcef = add_(p0Ecef_, scale_(dirEcef, distance));

	const double theta = kEarthRotationRate * t;
	const Mat3 ecefToEci = ecef_to_eci_matrix_(theta);

	center = mat_vec_(ecefToEci, centerEcef);

	// body -> NED。
	const Mat3 bodyToNed = rpy_matrix_(roll, pitch, yaw);

	// NED -> ECEF。
	const Mat3 nedToEcef = ned_to_ecef_matrix_(northEcef_, eastEcef_, upEcef_);

	// body -> ECEF -> ECI。
	bodyToGlobal = mat_mul_(ecefToEci, mat_mul_(nedToEcef, bodyToNed));
}

void RADAR_CornerReflectorLocation::write_outputs_(const std::vector<Vec3>& positions,
	const std::vector<double>& rcsValues)
{
	const int nParam = (NumberOfCornerReflector > 0) ? NumberOfCornerReflector : 1;

	const int nPosBus = Pos.GetSize();
	const int nRcsBus = CornerRCS.GetSize();

	const int nPosWrite = std::min(nParam, nPosBus);
	const int nRcsWrite = std::min(nParam, nRcsBus);

	for (int i = 0; i < nPosWrite; ++i)
	{
		const Vec3 p = (i < static_cast<int>(positions.size()))
			? positions[static_cast<std::size_t>(i)]
			: make_vec_(0.0, 0.0, 0.0);

		SystemVueModelBuilder::DoubleMatrix m(3, 1);
		m(0, 0) = p.x;
		m(1, 0) = p.y;
		m(2, 0) = p.z;

		Pos[i][0U] = m;
	}

	for (int i = 0; i < nRcsWrite; ++i)
	{
		const double r = (i < static_cast<int>(rcsValues.size()))
			? rcsValues[static_cast<std::size_t>(i)]
			: 0.0;

		CornerRCS[i][0U] = r;
	}
}

bool RADAR_CornerReflectorLocation::Setup()
{
	if (NumberOfCornerReflector <= 0)
		NumberOfCornerReflector = 1;

	sampleIndex_ = 0ULL;
	userPathIndex_ = 0U;

	if (!validate_params_())
		return false;

	// 初始化 ECI 模式使用的 LLA/ECEF 基。
	const double lonDeg = get_array_value_(Position_Initial, Position_InitialSize, 0, 0.0);
	const double latDeg = get_array_value_(Position_Initial, Position_InitialSize, 1, 0.0);
	const double h = get_array_value_(Position_Initial, Position_InitialSize, 2, 0.0);

	lonRad_ = lonDeg * kDegToRad;
	latRad_ = latDeg * kDegToRad;

	p0Ecef_ = lla_to_ecef_(lonRad_, latRad_, h);

	const double sinLon = std::sin(lonRad_);
	const double cosLon = std::cos(lonRad_);
	const double sinLat = std::sin(latRad_);
	const double cosLat = std::cos(latRad_);

	eastEcef_ = make_vec_(-sinLon, cosLon, 0.0);
	northEcef_ = make_vec_(-sinLat * cosLon, -sinLat * sinLon, cosLat);
	upEcef_ = make_vec_(cosLat * cosLon, cosLat * sinLon, sinLat);

	load_user_file_();

	return true;
}

bool RADAR_CornerReflectorLocation::Run()
{
	const int n = (NumberOfCornerReflector > 0) ? NumberOfCornerReflector : 1;

	// ------------------------------------------------------------------------
	// UserDefined 模式：
	// 文件中已经给出每个角反射器的位置和 RCS。
	// 这种模式通常用于导入外部全波仿真/实测 RCS 表或轨迹结果。
	// ------------------------------------------------------------------------
	if (Trajectory_Mode == User_Defined)
	{
		UserSample out = lastUserSample_;

		if (!userPath_.empty())
		{
			if (userPathIndex_ < userPath_.size())
			{
				out = userPath_[userPathIndex_];
				lastUserSample_ = out;
				++userPathIndex_;
			}
			else
			{
				out = lastUserSample_;
			}
		}

		write_outputs_(out.pos, out.rcs);
		++sampleIndex_;
		return true;
	}

	Vec3 center = make_vec_(0.0, 0.0, 0.0);
	Mat3 modelBodyToGlobal = identity_mat_();

	if (Trajectory_Mode == SimpleXYZ_Frame)
	{
		compute_simple_xyz_center_and_orientation_(center, modelBodyToGlobal);
	}
	else
	{
		compute_eci_center_and_orientation_(center, modelBodyToGlobal);
	}

	const double freqHz = get_carrier_freq_();

	std::vector<Vec3> positions;
	std::vector<double> rcsValues;

	positions.reserve(static_cast<std::size_t>(n));
	rcsValues.reserve(static_cast<std::size_t>(n));

	for (int i = 0; i < n; ++i)
	{
		// 每个角反射器相对质心的位置偏移，默认在模型 body frame 下定义。
		const Vec3 locBody = get_corner_loc_(i);

		const double rollOffset = get_array_broadcast_(CornerRollOffset, CornerRollOffsetSize, i, 0.0) * kDegToRad;
		const double pitchOffset = get_array_broadcast_(CornerPitchOffset, CornerPitchOffsetSize, i, 0.0) * kDegToRad;
		const double yawOffset = get_array_broadcast_(CornerYawOffset, CornerYawOffsetSize, i, 0.0) * kDegToRad;

		const Mat3 cornerOffsetRot = rpy_matrix_(rollOffset, pitchOffset, yawOffset);

		// 单个角反射器的 body->global 姿态。
		const Mat3 cornerBodyToGlobal = mat_mul_(modelBodyToGlobal, cornerOffsetRot);

		// 位置由质心 + 被姿态旋转后的安装偏移得到。
		Vec3 reflectorPos = add_(center, mat_vec_(modelBodyToGlobal, locBody));

		// 等效相位中心偏移。
		// 第一版默认 [0 0 0]；如果用户已知相位中心，可在 body frame 中给出。
		const Vec3 phaseOffsetBody = get_phase_center_offset_(i);
		reflectorPos = add_(reflectorPos, mat_vec_(cornerBodyToGlobal, phaseOffsetBody));

		const double sigma = calc_corner_rcs_(i, reflectorPos, cornerBodyToGlobal, freqHz);

		positions.push_back(reflectorPos);
		rcsValues.push_back(sigma);
	}

	write_outputs_(positions, rcsValues);

	++sampleIndex_;
	return true;
}