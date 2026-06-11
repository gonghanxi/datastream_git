#include "RADAR_AngleTransform.h"

#include <cmath>

#ifndef M_PI
#define M_PI 3.141592653589793238462643383279502884
#endif

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(RADAR_AngleTransform)
{
	SET_MODEL_DESCRIPTION("Angle Transformation between different antenna frame");
	SET_MODEL_CATEGORY("Array TR");

	// ============================================================
	// 端口
	// ============================================================
	{
		auto p = ADD_MODEL_INPUT(inEl_inTheta);
		p.SetName("inEl_inTheta");
		p.SetDescription("When TransformType is set as From Antenna Coordinates to Radar Coordinates, it is the input Theta angle in the source Antenna Coordinates. Otherwise, it is the input Elevation angle in the source Radar Coordinates. Units: radian.");
	}

	{
		auto p = ADD_MODEL_INPUT(inAz_inPhi);
		p.SetName("inAz_inPhi");
		p.SetDescription("When TransformType is set as From Antenna Coordinates to Radar Coordinates, it is the input Phi angle in the source Antenna Coordinates. Otherwise, it is the input Azimuth angle in the source Radar Coordinates. Units: radian.");
	}

	{
		auto p = ADD_MODEL_OUTPUT(outEl_outTheta);
		p.SetName("outEl_outTheta");
		p.SetDescription("When TransformType is set as From Antenna Coordinates to Radar Coordinates, it is the output Elevation angle in the source Radar Coordinates. Otherwise, it is the output Theta angle in the source Antenna Coordinates. Units: radian.");
	}

	{
		auto p = ADD_MODEL_OUTPUT(outAz_outPhi);
		p.SetName("outAz_outPhi");
		p.SetDescription("When TransformType is set as From Antenna Coordinates to Radar Coordinates, it is the output Azimuth angle in the source Radar Coordinates. Otherwise, it is the output Phi angle in the source Antenna Coordinates. Units: radian.");
	}

	// ============================================================
	// 参数
	// ============================================================
	{
		auto p = ADD_MODEL_ENUM_PARAM(TransformType, RADAR_AngleTransform::TransformTypeEnum);
		p.SetName("TransformType");
		p.AddEnumeration("From Antenna Coordinates to Radar Coordinates",
			RADAR_AngleTransform::From_Antenna_Coordinates_to_Radar_Coordinates);
		p.AddEnumeration("From Radar Coordinates to Antenna Coordinates",
			RADAR_AngleTransform::From_Radar_Coordinates_to_Antenna_Coordinates);
		p.SetDefaultValue("From Antenna Coordinates to Radar Coordinates");
		p.SetDescription("The type of angle transformation between two coordinates: From Antenna Coordinates to Radar Coordinates, From Radar Coordinates to Antenna Coordinates");
	}

	return true;
}
#endif

// ============================================================
// 构造函数
// ============================================================
RADAR_AngleTransform::RADAR_AngleTransform()
	: inEl_inTheta()
	, inAz_inPhi()
	, outEl_outTheta()
	, outAz_outPhi()
	, TransformType(From_Antenna_Coordinates_to_Radar_Coordinates)
{
}

bool RADAR_AngleTransform::Setup()
{
	// 帮助文档没有多速率说明；每次 firing 读取一组角度，输出一组角度。
	inEl_inTheta.SetRate(1u);
	inAz_inPhi.SetRate(1u);
	outEl_outTheta.SetRate(1u);
	outAz_outPhi.SetRate(1u);

	return true;
}

bool RADAR_AngleTransform::Run()
{
	const double a1 = sanitize_(inEl_inTheta[0], 0.0);
	const double a2 = sanitize_(inAz_inPhi[0], 0.0);

	double out1 = 0.0;
	double out2 = 0.0;

	if (TransformType == From_Radar_Coordinates_to_Antenna_Coordinates)
	{
		// 输入：a1 = elevation(theta_EL), a2 = azimuth(theta_AZ)，单位 radian
		// 输出：out1 = theta_z, out2 = phi，单位 radian
		radarToAntenna_(a1, a2, out1, out2);
	}
	else
	{
		// 默认：From Antenna Coordinates to Radar Coordinates
		// 输入：a1 = theta_z, a2 = phi，单位 radian
		// 输出：out1 = elevation(theta_EL), out2 = azimuth(theta_AZ)，单位 radian
		antennaToRadar_(a1, a2, out1, out2);
	}

	outEl_outTheta[0] = out1;
	outAz_outPhi[0] = out2;

	return true;
}

void RADAR_AngleTransform::antennaToRadar_(double theta,
	double phi,
	double& elevation,
	double& azimuth) const
{
	// Antenna Coordinates:
	//   theta：从 +Z 轴到目标方向 R 的夹角
	//   phi  ：R 在 X-Y 平面投影与 +X 轴之间的夹角
	// 单位方向向量：
	//   x = sin(theta) * cos(phi)
	//   y = sin(theta) * sin(phi)
	//   z = cos(theta)
	const double st = std::sin(theta);
	const double x = st * std::cos(phi);
	const double y = st * std::sin(phi);
	const double z = std::cos(theta);

	// Radar Coordinates:
	//   azimuth   ：R 在 X-Z 平面投影与 +Z 轴之间的夹角
	//   elevation ：R 与 X-Z 平面之间的夹角
	elevation = std::asin(clampUnit_(y));
	azimuth = std::atan2(x, z);
}

void RADAR_AngleTransform::radarToAntenna_(double elevation,
	double azimuth,
	double& theta,
	double& phi) const
{
	// Radar Coordinates:
	//   elevation：R 与 X-Z 平面之间的夹角
	//   azimuth  ：R 在 X-Z 平面投影与 +Z 轴之间的夹角
	// 单位方向向量：
	//   x = sin(azimuth) * cos(elevation)
	//   y = sin(elevation)
	//   z = cos(azimuth) * cos(elevation)
	const double ce = std::cos(elevation);
	const double x = std::sin(azimuth) * ce;
	const double y = std::sin(elevation);
	const double z = std::cos(azimuth) * ce;

	// Antenna Coordinates:
	//   theta：从 +Z 轴到目标方向 R 的夹角
	//   phi  ：R 在 X-Y 平面投影与 +X 轴之间的夹角
	theta = std::acos(clampUnit_(z));
	phi = std::atan2(y, x);
}

double RADAR_AngleTransform::clampUnit_(double x)
{
	if (x > 1.0)  return 1.0;
	if (x < -1.0) return -1.0;
	return x;
}

double RADAR_AngleTransform::sanitize_(double x, double fallback)
{
	if (x != x) {
		return fallback;
	}

	if (!std::isfinite(x)) {
		return fallback;
	}

	return x;
}
