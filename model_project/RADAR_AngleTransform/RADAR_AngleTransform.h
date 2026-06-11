#pragma once

#include "ModelBuilder.h"
#include "DFModel.h"
#include "CircularBuffer.h"

#include <cmath>

class SYSTEMVUEMODELBUILDER_API RADAR_AngleTransform : public SystemVueModelBuilder::DFModel
{
public:
	DECLARE_MODEL_INTERFACE(RADAR_AngleTransform);

	RADAR_AngleTransform();

	bool Setup() override;
	bool Run() override;

	// ============================================================
	// TransformType 参数枚举
	// 0：Antenna Coordinates(theta_z, phi) -> Radar Coordinates(elevation, azimuth)
	// 1：Radar Coordinates(elevation, azimuth) -> Antenna Coordinates(theta_z, phi)
	// ============================================================
	enum TransformTypeEnum
	{
		From_Antenna_Coordinates_to_Radar_Coordinates = 0,
		From_Radar_Coordinates_to_Antenna_Coordinates = 1
	};

	// ============================================================
	// 端口定义
	// Port 1：inEl_inTheta
	//   TransformType = Antenna -> Radar 时：输入 theta_z，单位 radian
	//   TransformType = Radar   -> Antenna 时：输入 elevation，单位 radian
	//
	// Port 2：inAz_inPhi
	//   TransformType = Antenna -> Radar 时：输入 phi，单位 radian
	//   TransformType = Radar   -> Antenna 时：输入 azimuth，单位 radian
	//
	// Port 3：outEl_outTheta
	//   TransformType = Antenna -> Radar 时：输出 elevation，单位 radian
	//   TransformType = Radar   -> Antenna 时：输出 theta_z，单位 radian
	//
	// Port 4：outAz_outPhi
	//   TransformType = Antenna -> Radar 时：输出 azimuth，单位 radian
	//   TransformType = Radar   -> Antenna 时：输出 phi，单位 radian
	// ============================================================
	SystemVueModelBuilder::DoubleCircularBuffer inEl_inTheta;
	SystemVueModelBuilder::DoubleCircularBuffer inAz_inPhi;
	SystemVueModelBuilder::DoubleCircularBuffer outEl_outTheta;
	SystemVueModelBuilder::DoubleCircularBuffer outAz_outPhi;

	// ============================================================
	// RADAR_AngleTransform 帮助文档参数
	// ============================================================
	TransformTypeEnum TransformType;

private:
	void antennaToRadar_(double theta, double phi,
		double& elevation, double& azimuth) const;

	void radarToAntenna_(double elevation, double azimuth,
		double& theta, double& phi) const;

	static double clampUnit_(double x);
	static double sanitize_(double x, double fallback);
};
