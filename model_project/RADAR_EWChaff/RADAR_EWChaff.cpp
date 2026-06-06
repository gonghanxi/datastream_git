#include "RADAR_EWChaff.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

const double RADAR_EWChaff::kPi = 3.1415926535897932384626433832795;
const double RADAR_EWChaff::kSpeedOfLight = 299792458.0;
const double RADAR_EWChaff::kGoldenAngle = 2.39996322972865332223;

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(RADAR_EWChaff)
{
	SET_MODEL_DESCRIPTION("Radar EW Chaff Equivalent Cloud Model");
	SET_MODEL_CATEGORY("EW");

	// --------- 输入端口 ---------
	{
		auto p = ADD_MODEL_INPUT(Release);
		p.SetDescription("Release control signal. Release > 0 triggers chaff deployment.");
		p.SetOptional(true);
	}
	{
		auto p = ADD_MODEL_INPUT(PlatformX);
		p.SetDescription("Platform X position at release, unit m.");
		p.SetOptional(true);
	}
	{
		auto p = ADD_MODEL_INPUT(PlatformY);
		p.SetDescription("Platform Y position at release, unit m.");
		p.SetOptional(true);
	}
	{
		auto p = ADD_MODEL_INPUT(PlatformZ);
		p.SetDescription("Platform Z position at release, unit m.");
		p.SetOptional(true);
	}
	{
		auto p = ADD_MODEL_INPUT(PlatformVx);
		p.SetDescription("Platform X velocity at release, unit m/s.");
		p.SetOptional(true);
	}
	{
		auto p = ADD_MODEL_INPUT(PlatformVy);
		p.SetDescription("Platform Y velocity at release, unit m/s.");
		p.SetOptional(true);
	}
	{
		auto p = ADD_MODEL_INPUT(PlatformVz);
		p.SetDescription("Platform Z velocity at release, unit m/s.");
		p.SetOptional(true);
	}
	{
		auto p = ADD_MODEL_INPUT(CarrierFreqIn);
		p.SetDescription("Carrier frequency input in Hz. If disconnected or <=0, CarrierFreq parameter is used.");
		p.SetOptional(true);
	}

	// --------- 输出端口 ---------
	{
		auto p = ADD_MODEL_OUTPUT(ChaffPos);
		p.SetDescription("Equivalent chaff cloud cell position. Each bus channel is a 3x1 real matrix [X;Y;Z].");
	}
	{
		auto p = ADD_MODEL_OUTPUT(ChaffRCS);
		p.SetDescription("Equivalent chaff cloud cell RCS. Unit is m^2 if Linear_m2, or dBsm if dBsm.");
	}
	{
		auto p = ADD_MODEL_OUTPUT(ChaffVel);
		p.SetDescription("Equivalent chaff cloud cell velocity. Each bus channel is a 3x1 real matrix [Vx;Vy;Vz].");
	}
	{
		auto p = ADD_MODEL_OUTPUT(ValidFlag);
		p.SetDescription("Validity flag of each chaff cloud cell. 1 means valid, 0 means inactive.");
	}

	// --------- 模式参数 ---------
	{
		auto p = ADD_MODEL_ENUM_PARAM(Chaff_Mode, Chaff_ModeEnum);
		p.AddEnumeration("FixedCloud", RADAR_EWChaff::FixedCloud);
		p.AddEnumeration("ReleasedCloud", RADAR_EWChaff::ReleasedCloud);
		p.AddEnumeration("UserDefined", RADAR_EWChaff::User_Defined);
		p.SetDefaultValue("ReleasedCloud");
		p.SetDescription("Chaff cloud mode.");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(FileName);
		p.SetDefaultValue("");
		p.SetDescription("UserDefined file path. Each line format: x y z rcs vx vy vz valid for each chaff cell.");
		p.SetParamAsFile();
		p.SetHideCondition("Chaff_Mode ~= 2");
	}

	{
		auto p = ADD_MODEL_ENUM_PARAM(Cloud_Model, Cloud_ModelEnum);
		p.AddEnumeration("SingleCell", RADAR_EWChaff::SingleCell);
		p.AddEnumeration("MultiCell", RADAR_EWChaff::MultiCell);
		p.SetDefaultValue("MultiCell");
		p.SetDescription("SingleCell uses one equivalent scatterer. MultiCell splits cloud into multiple cells.");
		p.SetHideCondition("Chaff_Mode == 2");
	}

	{
		auto p = ADD_MODEL_PARAM(NumberOfChaffCell);
		p.SetDefaultValue("16");
		p.SetDescription("Number of equivalent chaff cloud cells. In SingleCell mode, only one cell is used internally.");
	}

	{
		auto p = ADD_MODEL_ENUM_PARAM(RCS_Model, RCSModelEnum);
		p.AddEnumeration("EmpiricalTotalRCS", RADAR_EWChaff::EmpiricalTotalRCS);
		p.AddEnumeration("ResonantDipoleApprox", RADAR_EWChaff::ResonantDipoleApprox);
		p.SetDefaultValue("ResonantDipoleApprox");
		p.SetDescription("RCS model. ResonantDipoleApprox adds half-wave dipole length matching.");
		p.SetHideCondition("Chaff_Mode == 2");
	}

	{
		auto p = ADD_MODEL_ENUM_PARAM(RCS_OutputUnit, RCSOutputUnitEnum);
		p.AddEnumeration("Linear_m2", RADAR_EWChaff::Linear_m2);
		p.AddEnumeration("dBsm", RADAR_EWChaff::dBsm);
		p.SetDefaultValue("Linear_m2");
		p.SetDescription("RCS output unit.");
	}

	{
		auto p = ADD_MODEL_ENUM_PARAM(Cell_RCS_Distribution, CellRCSDistributionEnum);
		p.AddEnumeration("Uniform_Distribution", RADAR_EWChaff::Uniform_Distribution);
		p.AddEnumeration("GaussianCenter_Distribution", RADAR_EWChaff::GaussianCenter_Distribution);
		p.SetDefaultValue("GaussianCenter_Distribution");
		p.SetDescription("How total cloud RCS is distributed among cells.");
		p.SetHideCondition("Chaff_Mode == 2 || Cloud_Model == 0");
	}

	// --------- 位置/速度/风速参数 ---------
	{
		auto p = ADD_MODEL_ARRAY_PARAM(Release_Position_XYZ, Release_Position_XYZSize);
		p.SetDefaultValue("[0 0 0]");
		p.SetUnit(SystemVueModelBuilder::Units::LENGTH);
		p.SetDescription("Fallback release position or fixed cloud center [X Y Z], unit m.");
		p.SetHideCondition("Chaff_Mode == 2");
	}

	{
		auto p = ADD_MODEL_ARRAY_PARAM(Initial_Velocity_XYZ, Initial_Velocity_XYZSize);
		p.SetDefaultValue("[0 0 0]");
		p.SetDescription("Fallback initial release velocity if PlatformVx/Vy/Vz are disconnected, unit m/s.");
		p.SetHideCondition("Chaff_Mode ~= 1");
	}

	{
		auto p = ADD_MODEL_ARRAY_PARAM(Wind_Velocity_XYZ, Wind_Velocity_XYZSize);
		p.SetDefaultValue("[0 0 0]");
		p.SetDescription("Wind velocity of chaff cloud drift, unit m/s.");
		p.SetHideCondition("Chaff_Mode ~= 1");
	}

	{
		auto p = ADD_MODEL_PARAM(Fall_Speed);
		p.SetDefaultValue("0");
		p.SetDescription("Average downward fall speed along -Z direction, unit m/s.");
		p.SetHideCondition("Chaff_Mode ~= 1");
	}

	{
		auto p = ADD_MODEL_PARAM(VelocityDecayTime);
		p.SetDefaultValue("0.5");
		p.SetUnit(SystemVueModelBuilder::Units::TIME);
		p.SetDescription("Time constant of platform velocity memory decay, unit s.");
		p.SetHideCondition("Chaff_Mode ~= 1");
	}

	// --------- 云团扩散参数 ---------
	{
		auto p = ADD_MODEL_ARRAY_PARAM(Cloud_Initial_Radius_XYZ, Cloud_Initial_Radius_XYZSize);
		p.SetDefaultValue("[5 5 5]");
		p.SetUnit(SystemVueModelBuilder::Units::LENGTH);
		p.SetDescription("Initial cloud radius [Rx Ry Rz], unit m. Can also be a single value.");
		p.SetHideCondition("Chaff_Mode == 2");
	}

	{
		auto p = ADD_MODEL_ARRAY_PARAM(Cloud_ExpansionRate_XYZ, Cloud_ExpansionRate_XYZSize);
		p.SetDefaultValue("[2 2 1]");
		p.SetDescription("Cloud expansion rate [dRx dRy dRz], unit m/s. Can also be a single value.");
		p.SetHideCondition("Chaff_Mode ~= 1 || Cloud_Model == 0");
	}

	{
		auto p = ADD_MODEL_ARRAY_PARAM(Cloud_MaxRadius_XYZ, Cloud_MaxRadius_XYZSize);
		p.SetDefaultValue("[100 100 60]");
		p.SetUnit(SystemVueModelBuilder::Units::LENGTH);
		p.SetDescription("Maximum cloud radius [Rx Ry Rz], unit m. <=0 means no cap.");
		p.SetHideCondition("Chaff_Mode ~= 1 || Cloud_Model == 0");
	}

	{
		auto p = ADD_MODEL_PARAM(Cloud_Lifetime);
		p.SetDefaultValue("30");
		p.SetUnit(SystemVueModelBuilder::Units::TIME);
		p.SetDescription("Effective cloud lifetime, unit s. <=0 means no lifetime shutoff.");
		p.SetHideCondition("Chaff_Mode ~= 1");
	}

	// --------- 箔条物理/RCS 参数 ---------
	{
		auto p = ADD_MODEL_PARAM(CarrierFreq);
		p.SetDefaultValue("10e9");
		p.SetDescription("Carrier frequency in Hz. Wavelength lambda = c / CarrierFreq.");
		p.SetHideCondition("Chaff_Mode == 2");
	}

	{
		auto p = ADD_MODEL_PARAM(DipoleLength);
		p.SetDefaultValue("0.015");
		p.SetUnit(SystemVueModelBuilder::Units::LENGTH);
		p.SetDescription("Equivalent chaff dipole length. Strong scattering occurs when close to lambda/2.");
		p.SetHideCondition("Chaff_Mode == 2 || RCS_Model ~= 1");
	}

	{
		auto p = ADD_MODEL_PARAM(DipoleLengthSpread);
		p.SetDefaultValue("0.003");
		p.SetUnit(SystemVueModelBuilder::Units::LENGTH);
		p.SetDescription("Length matching spread. Larger value means wider frequency coverage.");
		p.SetHideCondition("Chaff_Mode == 2 || RCS_Model ~= 1");
	}

	{
		auto p = ADD_MODEL_PARAM(NumberOfDipoles);
		p.SetDefaultValue("100000");
		p.SetDescription("Number of chaff dipoles, used as scaling factor in ResonantDipoleApprox.");
		p.SetHideCondition("Chaff_Mode == 2 || RCS_Model ~= 1");
	}

	{
		auto p = ADD_MODEL_PARAM(ReferenceDipoleCount);
		p.SetDefaultValue("100000");
		p.SetDescription("Reference dipole count corresponding to TotalRCS_Reference.");
		p.SetHideCondition("Chaff_Mode == 2 || RCS_Model ~= 1");
	}

	{
		auto p = ADD_MODEL_PARAM(TotalRCS_Reference);
		p.SetDefaultValue("1000");
		p.SetDescription("Reference peak total RCS of whole chaff cloud in m^2.");
		p.SetHideCondition("Chaff_Mode == 2");
	}

	{
		auto p = ADD_MODEL_PARAM(RCS_GrowthTime);
		p.SetDefaultValue("0.2");
		p.SetUnit(SystemVueModelBuilder::Units::TIME);
		p.SetDescription("RCS growth time after release, unit s.");
		p.SetHideCondition("Chaff_Mode ~= 1");
	}

	{
		auto p = ADD_MODEL_PARAM(RCS_DecayTime);
		p.SetDefaultValue("20");
		p.SetUnit(SystemVueModelBuilder::Units::TIME);
		p.SetDescription("RCS decay time, unit s.");
		p.SetHideCondition("Chaff_Mode ~= 1");
	}

	{
		auto p = ADD_MODEL_PARAM(RCS_Floor);
		p.SetDefaultValue("0");
		p.SetDescription("Minimum active cloud RCS in m^2.");
		p.SetHideCondition("Chaff_Mode == 2");
	}

	{
		auto p = ADD_MODEL_PARAM(GaussianWeightSigma);
		p.SetDefaultValue("0.65");
		p.SetDescription("Gaussian center weight sigma for cell RCS distribution.");
		p.SetHideCondition("Chaff_Mode == 2 || Cloud_Model == 0 || Cell_RCS_Distribution ~= 1");
	}

	{
		auto p = ADD_MODEL_PARAM(TimeStep);
		p.SetDefaultValue("1e-3");
		p.SetUnit(SystemVueModelBuilder::Units::TIME);
		p.SetDescription("Internal time step, unit s.");
	}

	return true;
}
#endif // SV_CODE_GEN

RADAR_EWChaff::RADAR_EWChaff()
	: Chaff_Mode(ReleasedCloud)
	, Cloud_Model(MultiCell)
	, RCS_Model(ResonantDipoleApprox)
	, RCS_OutputUnit(Linear_m2)
	, Cell_RCS_Distribution(GaussianCenter_Distribution)
	, NumberOfChaffCell(16)
	, FileName(nullptr)
	, Release_Position_XYZ(nullptr)
	, Release_Position_XYZSize(0)
	, Initial_Velocity_XYZ(nullptr)
	, Initial_Velocity_XYZSize(0)
	, Wind_Velocity_XYZ(nullptr)
	, Wind_Velocity_XYZSize(0)
	, Fall_Speed(0.0)
	, VelocityDecayTime(0.5)
	, Cloud_Initial_Radius_XYZ(nullptr)
	, Cloud_Initial_Radius_XYZSize(0)
	, Cloud_ExpansionRate_XYZ(nullptr)
	, Cloud_ExpansionRate_XYZSize(0)
	, Cloud_MaxRadius_XYZ(nullptr)
	, Cloud_MaxRadius_XYZSize(0)
	, Cloud_Lifetime(30.0)
	, CarrierFreq(10e9)
	, DipoleLength(0.015)
	, DipoleLengthSpread(0.003)
	, NumberOfDipoles(100000.0)
	, ReferenceDipoleCount(100000.0)
	, TotalRCS_Reference(1000.0)
	, RCS_GrowthTime(0.2)
	, RCS_DecayTime(20.0)
	, RCS_Floor(0.0)
	, GaussianWeightSigma(0.65)
	, TimeStep(1e-3)
	, sampleIndex_(0ULL)
	, released_(false)
	, releaseSampleIndex_(0ULL)
	, releasePosLatched_(make_vec_(0.0, 0.0, 0.0))
	, releaseVelLatched_(make_vec_(0.0, 0.0, 0.0))
	, userPathIndex_(0U)
{
	init_empty_user_sample_();
}

double RADAR_EWChaff::get_array_value_(const double* p, int size, int idx, double defval)
{
	if (p == nullptr || idx < 0 || idx >= size)
		return defval;
	return p[idx];
}

RADAR_EWChaff::Vec3 RADAR_EWChaff::get_array_vec3_(const double* p, int size, double defx, double defy, double defz)
{
	if (p == nullptr || size <= 0)
		return make_vec_(defx, defy, defz);

	if (size == 1)
		return make_vec_(p[0], p[0], p[0]);

	return make_vec_(
		get_array_value_(p, size, 0, defx),
		get_array_value_(p, size, 1, defy),
		get_array_value_(p, size, 2, defz)
	);
}

RADAR_EWChaff::Vec3 RADAR_EWChaff::make_vec_(double x, double y, double z)
{
	Vec3 v;
	v.x = x;
	v.y = y;
	v.z = z;
	return v;
}

RADAR_EWChaff::Vec3 RADAR_EWChaff::add_(const Vec3& a, const Vec3& b)
{
	return make_vec_(a.x + b.x, a.y + b.y, a.z + b.z);
}

RADAR_EWChaff::Vec3 RADAR_EWChaff::sub_(const Vec3& a, const Vec3& b)
{
	return make_vec_(a.x - b.x, a.y - b.y, a.z - b.z);
}

RADAR_EWChaff::Vec3 RADAR_EWChaff::scale_(const Vec3& a, double s)
{
	return make_vec_(a.x * s, a.y * s, a.z * s);
}

RADAR_EWChaff::Vec3 RADAR_EWChaff::mul_(const Vec3& a, const Vec3& b)
{
	return make_vec_(a.x * b.x, a.y * b.y, a.z * b.z);
}

double RADAR_EWChaff::dot_(const Vec3& a, const Vec3& b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

double RADAR_EWChaff::norm_(const Vec3& a)
{
	return std::sqrt(dot_(a, a));
}

RADAR_EWChaff::Vec3 RADAR_EWChaff::normalize_(const Vec3& a, const Vec3& fallback)
{
	const double n = norm_(a);
	if (n <= 0.0)
		return fallback;
	return scale_(a, 1.0 / n);
}

double RADAR_EWChaff::safe_log10_(double x)
{
	const double eps = 1.0e-300;
	return std::log10((x > eps) ? x : eps);
}

int RADAR_EWChaff::active_cell_count_() const
{
	if (Chaff_Mode == User_Defined)
		return (NumberOfChaffCell > 0) ? NumberOfChaffCell : 1;

	if (Cloud_Model == SingleCell)
		return 1;

	return (NumberOfChaffCell > 0) ? NumberOfChaffCell : 1;
}

bool RADAR_EWChaff::validate_params_() const
{
	if (NumberOfChaffCell <= 0)
	{
		std::cerr << "RADAR_EWChaff: NumberOfChaffCell should be positive." << std::endl;
		return false;
	}

	if (TimeStep <= 0.0)
	{
		std::cerr << "RADAR_EWChaff: TimeStep should be positive." << std::endl;
		return false;
	}

	if (Chaff_Mode != User_Defined)
	{
		if (CarrierFreq <= 0.0)
		{
			std::cerr << "RADAR_EWChaff: CarrierFreq should be positive." << std::endl;
			return false;
		}

		if (TotalRCS_Reference < 0.0)
		{
			std::cerr << "RADAR_EWChaff: TotalRCS_Reference should be non-negative." << std::endl;
			return false;
		}

		if (ReferenceDipoleCount <= 0.0)
		{
			std::cerr << "RADAR_EWChaff: ReferenceDipoleCount should be positive." << std::endl;
			return false;
		}

		if (GaussianWeightSigma <= 0.0)
		{
			std::cerr << "RADAR_EWChaff: GaussianWeightSigma should be positive." << std::endl;
			return false;
		}
	}

	return true;
}

void RADAR_EWChaff::init_empty_user_sample_()
{
	const int n = active_cell_count_();

	lastUserSample_.pos.assign(static_cast<std::size_t>(n), make_vec_(0.0, 0.0, 0.0));
	lastUserSample_.rcs.assign(static_cast<std::size_t>(n), 0.0);
	lastUserSample_.vel.assign(static_cast<std::size_t>(n), make_vec_(0.0, 0.0, 0.0));
	lastUserSample_.valid.assign(static_cast<std::size_t>(n), 0.0);
}

bool RADAR_EWChaff::load_user_file_()
{
	userPath_.clear();
	userPathIndex_ = 0U;
	init_empty_user_sample_();

	const int n = active_cell_count_();

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
		sample.vel.reserve(static_cast<std::size_t>(n));
		sample.valid.reserve(static_cast<std::size_t>(n));

		bool ok = true;
		for (int i = 0; i < n; ++i)
		{
			double x = 0.0, y = 0.0, z = 0.0;
			double rcs = 0.0;
			double vx = 0.0, vy = 0.0, vz = 0.0;
			double valid = 0.0;

			if (!(iss >> x >> y >> z >> rcs >> vx >> vy >> vz >> valid))
			{
				ok = false;
				break;
			}

			sample.pos.push_back(make_vec_(x, y, z));
			sample.rcs.push_back(rcs);
			sample.vel.push_back(make_vec_(vx, vy, vz));
			sample.valid.push_back(valid);
		}

		if (ok && static_cast<int>(sample.pos.size()) == n)
			userPath_.push_back(sample);
	}

	if (!userPath_.empty())
		lastUserSample_ = userPath_.front();

	return true;
}

double RADAR_EWChaff::get_carrier_freq_()
{
	if (CarrierFreqIn.GetSize() > 0)
	{
		const double f = CarrierFreqIn[0];
		if (f > 0.0)
			return f;
	}

	return CarrierFreq;
}

RADAR_EWChaff::Vec3 RADAR_EWChaff::get_release_position_param_() const
{
	return get_array_vec3_(Release_Position_XYZ, Release_Position_XYZSize, 0.0, 0.0, 0.0);
}

RADAR_EWChaff::Vec3 RADAR_EWChaff::get_initial_velocity_param_() const
{
	return get_array_vec3_(Initial_Velocity_XYZ, Initial_Velocity_XYZSize, 0.0, 0.0, 0.0);
}

RADAR_EWChaff::Vec3 RADAR_EWChaff::get_platform_position_()
{
	Vec3 p = get_release_position_param_();

	if (PlatformX.GetSize() > 0) p.x = PlatformX[0];
	if (PlatformY.GetSize() > 0) p.y = PlatformY[0];
	if (PlatformZ.GetSize() > 0) p.z = PlatformZ[0];

	return p;
}

RADAR_EWChaff::Vec3 RADAR_EWChaff::get_platform_velocity_()
{
	Vec3 v = get_initial_velocity_param_();

	if (PlatformVx.GetSize() > 0) v.x = PlatformVx[0];
	if (PlatformVy.GetSize() > 0) v.y = PlatformVy[0];
	if (PlatformVz.GetSize() > 0) v.z = PlatformVz[0];

	return v;
}

RADAR_EWChaff::Vec3 RADAR_EWChaff::get_wind_velocity_() const
{
	return get_array_vec3_(Wind_Velocity_XYZ, Wind_Velocity_XYZSize, 0.0, 0.0, 0.0);
}

RADAR_EWChaff::Vec3 RADAR_EWChaff::get_initial_radius_() const
{
	return get_array_vec3_(Cloud_Initial_Radius_XYZ, Cloud_Initial_Radius_XYZSize, 5.0, 5.0, 5.0);
}

RADAR_EWChaff::Vec3 RADAR_EWChaff::get_expansion_rate_() const
{
	return get_array_vec3_(Cloud_ExpansionRate_XYZ, Cloud_ExpansionRate_XYZSize, 0.0, 0.0, 0.0);
}

RADAR_EWChaff::Vec3 RADAR_EWChaff::get_max_radius_() const
{
	return get_array_vec3_(Cloud_MaxRadius_XYZ, Cloud_MaxRadius_XYZSize, -1.0, -1.0, -1.0);
}

void RADAR_EWChaff::compute_cloud_center_velocity_radius_(double age,
	Vec3& center,
	Vec3& centerVel,
	Vec3& radius,
	Vec3& radiusRate) const
{
	const Vec3 initRadius = get_initial_radius_();
	const Vec3 expansion = get_expansion_rate_();
	const Vec3 maxRadius = get_max_radius_();

	if (Chaff_Mode == FixedCloud)
	{
		center = get_release_position_param_();
		centerVel = make_vec_(0.0, 0.0, 0.0);
		radius = initRadius;
		radiusRate = make_vec_(0.0, 0.0, 0.0);
		return;
	}

	const Vec3 windVel = get_wind_velocity_();

	const double tau = (VelocityDecayTime > 0.0) ? VelocityDecayTime : 0.0;

	Vec3 memoryDisp = make_vec_(0.0, 0.0, 0.0);
	Vec3 memoryVel = make_vec_(0.0, 0.0, 0.0);

	if (tau > 0.0)
	{
		const double e = std::exp(-age / tau);
		memoryDisp = scale_(releaseVelLatched_, tau * (1.0 - e));
		memoryVel = scale_(releaseVelLatched_, e);
	}

	const Vec3 windDisp = scale_(windVel, age);
	const Vec3 fallDisp = make_vec_(0.0, 0.0, -Fall_Speed * age);

	center = add_(add_(add_(releasePosLatched_, windDisp), memoryDisp), fallDisp);
	centerVel = add_(add_(windVel, memoryVel), make_vec_(0.0, 0.0, -Fall_Speed));

	Vec3 rawRadius = add_(initRadius, scale_(expansion, age));

	radius.x = (maxRadius.x > 0.0) ? std::min(rawRadius.x, maxRadius.x) : rawRadius.x;
	radius.y = (maxRadius.y > 0.0) ? std::min(rawRadius.y, maxRadius.y) : rawRadius.y;
	radius.z = (maxRadius.z > 0.0) ? std::min(rawRadius.z, maxRadius.z) : rawRadius.z;

	radiusRate.x = ((maxRadius.x > 0.0) && (rawRadius.x >= maxRadius.x)) ? 0.0 : expansion.x;
	radiusRate.y = ((maxRadius.y > 0.0) && (rawRadius.y >= maxRadius.y)) ? 0.0 : expansion.y;
	radiusRate.z = ((maxRadius.z > 0.0) && (rawRadius.z >= maxRadius.z)) ? 0.0 : expansion.z;
}

RADAR_EWChaff::Vec3 RADAR_EWChaff::cell_pattern_(int idx, int n) const
{
	if (n <= 1)
		return make_vec_(0.0, 0.0, 0.0);

	const double i = static_cast<double>(idx);
	const double N = static_cast<double>(n);

	const double z = 1.0 - 2.0 * (i + 0.5) / N;
	const double rxy = std::sqrt(std::max(0.0, 1.0 - z * z));
	const double phi = kGoldenAngle * i;

	const double frac = std::fmod((i + 1.0) * 0.6180339887498948482, 1.0);
	const double radial = std::pow(std::max(0.05, frac), 1.0 / 3.0);

	return make_vec_(
		radial * rxy * std::cos(phi),
		radial * rxy * std::sin(phi),
		radial * z
	);
}

void RADAR_EWChaff::compute_cell_weights_(const std::vector<Vec3>& patterns,
	std::vector<double>& weights) const
{
	const int n = static_cast<int>(patterns.size());
	weights.assign(static_cast<std::size_t>(n), 0.0);

	if (n <= 0)
		return;

	if (n == 1 || Cell_RCS_Distribution == Uniform_Distribution)
	{
		const double w = 1.0 / static_cast<double>(n);
		for (int i = 0; i < n; ++i)
			weights[static_cast<std::size_t>(i)] = w;
		return;
	}

	const double sigma = (GaussianWeightSigma > 0.0) ? GaussianWeightSigma : 0.65;
	double sumW = 0.0;

	for (int i = 0; i < n; ++i)
	{
		const double rho2 = dot_(patterns[static_cast<std::size_t>(i)],
			patterns[static_cast<std::size_t>(i)]);

		const double w = std::exp(-0.5 * rho2 / (sigma * sigma));
		weights[static_cast<std::size_t>(i)] = w;
		sumW += w;
	}

	if (sumW <= 0.0)
	{
		const double w = 1.0 / static_cast<double>(n);
		for (int i = 0; i < n; ++i)
			weights[static_cast<std::size_t>(i)] = w;
		return;
	}

	for (int i = 0; i < n; ++i)
		weights[static_cast<std::size_t>(i)] /= sumW;
}

double RADAR_EWChaff::frequency_match_factor_(double freqHz) const
{
	if (freqHz <= 0.0 || DipoleLength <= 0.0)
		return 0.0;

	const double lambda = kSpeedOfLight / freqHz;
	const double halfLambda = 0.5 * lambda;

	double spread = DipoleLengthSpread;
	if (spread <= 0.0)
		spread = std::max(1.0e-9, 0.05 * halfLambda);

	const double diff = DipoleLength - halfLambda;

	// 箔条等效为半波偶极子：长度越接近 lambda/2，散射越强。
	return std::exp(-(diff * diff) / (2.0 * spread * spread));
}

double RADAR_EWChaff::total_rcs_linear_(double age, double freqHz, bool active) const
{
	if (!active)
		return 0.0;

	double growth = 1.0;
	double decay = 1.0;

	if (Chaff_Mode == ReleasedCloud)
	{
		if (RCS_GrowthTime > 0.0)
			growth = 1.0 - std::exp(-age / RCS_GrowthTime);

		if (RCS_DecayTime > 0.0)
			decay = std::exp(-age / RCS_DecayTime);
	}

	double total = TotalRCS_Reference * growth * decay;

	if (RCS_Model == ResonantDipoleApprox)
	{
		const double match = frequency_match_factor_(freqHz);
		const double refCount = (ReferenceDipoleCount > 0.0) ? ReferenceDipoleCount : 1.0;
		const double countScale = std::max(0.0, NumberOfDipoles) / refCount;

		total *= match * countScale;
	}

	if (total > 0.0 && RCS_Floor > 0.0)
		total = std::max(total, RCS_Floor);

	if (total < 0.0)
		total = 0.0;

	return total;
}

void RADAR_EWChaff::write_outputs_(const std::vector<Vec3>& positions,
	const std::vector<double>& rcsValues,
	const std::vector<Vec3>& velocities,
	const std::vector<double>& validValues)
{
	const int nParam = active_cell_count_();

	const int nPosBus = ChaffPos.GetSize();
	const int nRcsBus = ChaffRCS.GetSize();
	const int nVelBus = ChaffVel.GetSize();
	const int nFlagBus = ValidFlag.GetSize();

	const int nPosWrite = std::min(nParam, nPosBus);
	const int nRcsWrite = std::min(nParam, nRcsBus);
	const int nVelWrite = std::min(nParam, nVelBus);
	const int nFlagWrite = std::min(nParam, nFlagBus);

	for (int i = 0; i < nPosWrite; ++i)
	{
		const Vec3 p = (i < static_cast<int>(positions.size()))
			? positions[static_cast<std::size_t>(i)]
			: make_vec_(0.0, 0.0, 0.0);

		SystemVueModelBuilder::DoubleMatrix m(3, 1);
		m(0, 0) = p.x;
		m(1, 0) = p.y;
		m(2, 0) = p.z;

		ChaffPos[i][0U] = m;
	}

	for (int i = 0; i < nRcsWrite; ++i)
	{
		const double r = (i < static_cast<int>(rcsValues.size()))
			? rcsValues[static_cast<std::size_t>(i)]
			: 0.0;

		ChaffRCS[i][0U] = r;
	}

	for (int i = 0; i < nVelWrite; ++i)
	{
		const Vec3 v = (i < static_cast<int>(velocities.size()))
			? velocities[static_cast<std::size_t>(i)]
			: make_vec_(0.0, 0.0, 0.0);

		SystemVueModelBuilder::DoubleMatrix m(3, 1);
		m(0, 0) = v.x;
		m(1, 0) = v.y;
		m(2, 0) = v.z;

		ChaffVel[i][0U] = m;
	}

	for (int i = 0; i < nFlagWrite; ++i)
	{
		const double f = (i < static_cast<int>(validValues.size()))
			? validValues[static_cast<std::size_t>(i)]
			: 0.0;

		ValidFlag[i][0U] = f;
	}
}

bool RADAR_EWChaff::Setup()
{
	if (NumberOfChaffCell <= 0)
		NumberOfChaffCell = 1;

	sampleIndex_ = 0ULL;
	userPathIndex_ = 0U;
	released_ = false;
	releaseSampleIndex_ = 0ULL;

	releasePosLatched_ = get_release_position_param_();
	releaseVelLatched_ = get_initial_velocity_param_();

	if (!validate_params_())
		return false;

	load_user_file_();

	return true;
}

bool RADAR_EWChaff::Run()
{
	const int n = active_cell_count_();

	if (Chaff_Mode == User_Defined)
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

		write_outputs_(out.pos, out.rcs, out.vel, out.valid);
		++sampleIndex_;
		return true;
	}

	if (Chaff_Mode == ReleasedCloud)
	{
		const bool releaseSignal = (Release.GetSize() > 0) ? (Release[0] > 0.0) : true;

		if (!released_ && releaseSignal)
		{
			released_ = true;
			releaseSampleIndex_ = sampleIndex_;

			// 关键：释放瞬间锁存平台位置和速度。
			// 后续即使平台继续运动，已经释放的箔条云也按锁存的释放点和初速演化。
			releasePosLatched_ = get_platform_position_();
			releaseVelLatched_ = get_platform_velocity_();
		}
	}
	else
	{
		released_ = true;
		releaseSampleIndex_ = 0ULL;
		releasePosLatched_ = get_release_position_param_();
		releaseVelLatched_ = make_vec_(0.0, 0.0, 0.0);
	}

	bool active = released_;
	double age = 0.0;

	if (active)
	{
		if (Chaff_Mode == ReleasedCloud)
			age = static_cast<double>(sampleIndex_ - releaseSampleIndex_) * TimeStep;
		else
			age = 0.0;
	}

	if (active && Chaff_Mode == ReleasedCloud && Cloud_Lifetime > 0.0 && age > Cloud_Lifetime)
		active = false;

	if (!active)
	{
		std::vector<Vec3> positions(static_cast<std::size_t>(n), make_vec_(0.0, 0.0, 0.0));
		std::vector<double> rcs(static_cast<std::size_t>(n), 0.0);
		std::vector<Vec3> vel(static_cast<std::size_t>(n), make_vec_(0.0, 0.0, 0.0));
		std::vector<double> valid(static_cast<std::size_t>(n), 0.0);

		write_outputs_(positions, rcs, vel, valid);
		++sampleIndex_;
		return true;
	}

	Vec3 center = make_vec_(0.0, 0.0, 0.0);
	Vec3 centerVel = make_vec_(0.0, 0.0, 0.0);
	Vec3 radius = make_vec_(0.0, 0.0, 0.0);
	Vec3 radiusRate = make_vec_(0.0, 0.0, 0.0);

	compute_cloud_center_velocity_radius_(age, center, centerVel, radius, radiusRate);

	std::vector<Vec3> patterns;
	patterns.reserve(static_cast<std::size_t>(n));

	for (int i = 0; i < n; ++i)
		patterns.push_back(cell_pattern_(i, n));

	std::vector<double> weights;
	compute_cell_weights_(patterns, weights);

	const double freqHz = get_carrier_freq_();
	const double totalRCS = total_rcs_linear_(age, freqHz, true);

	std::vector<Vec3> positions;
	std::vector<double> rcsValues;
	std::vector<Vec3> velocities;
	std::vector<double> validValues;

	positions.reserve(static_cast<std::size_t>(n));
	rcsValues.reserve(static_cast<std::size_t>(n));
	velocities.reserve(static_cast<std::size_t>(n));
	validValues.reserve(static_cast<std::size_t>(n));

	for (int i = 0; i < n; ++i)
	{
		const Vec3 pat = patterns[static_cast<std::size_t>(i)];

		const Vec3 offset = mul_(pat, radius);
		const Vec3 pos = add_(center, offset);

		// 单元速度 = 云团中心速度 + 扩散速度。
		const Vec3 expansionVel = mul_(pat, radiusRate);
		const Vec3 vel = add_(centerVel, expansionVel);

		double rcs = totalRCS * weights[static_cast<std::size_t>(i)];
		if (RCS_OutputUnit == dBsm)
			rcs = 10.0 * safe_log10_(rcs);

		positions.push_back(pos);
		rcsValues.push_back(rcs);
		velocities.push_back(vel);
		validValues.push_back(1.0);
	}

	write_outputs_(positions, rcsValues, velocities, validValues);

	++sampleIndex_;
	return true;
}