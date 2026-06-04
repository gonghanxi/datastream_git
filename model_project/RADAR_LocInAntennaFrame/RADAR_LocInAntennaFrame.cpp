#include "RADAR_LocInAntennaFrame.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( RADAR_LocInAntennaFrame )
{	
	SET_MODEL_DESCRIPTION("Target scatter location in the antenna frame");

	SET_MODEL_CATEGORY("Environments");
	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(RadarLoc);
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(TargetLoc);
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(BodyYaw);
		port.SetOptional();
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(BodyPitch);
		port.SetOptional();
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(BodyRoll);
		port.SetOptional();
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(AntYaw);
		port.SetOptional();
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(AntPitch);
		port.SetOptional();
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(AntRoll);
		port.SetOptional();
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(Elevation);
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(Azimuth);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(TimeStep);
		param.SetUnit(SystemVueModelBuilder::Units::TIME);
		param.SetDefaultValue("1e-9");
	}

	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(XYZFrameType, SelectedXYZFrameTypes);
		enumParam.AddEnumeration("ECI Frame", ECIFrame);
		enumParam.AddEnumeration("Simple XYZ Frame", XYZFrame);
		enumParam.SetDefaultValue("0");
	}

	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(AntennaPlaneType, SelectedAntennaPlaneTypes);
		enumParam.AddEnumeration("XY Plane", XYPlane);
		enumParam.AddEnumeration("YZ Plane", YZPlane);
		enumParam.SetDefaultValue("0");
	}

	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(CoordinateType, SelectedCoordinateTypes);
		enumParam.AddEnumeration("Radar Coordinate", RADARCoordinate);
		enumParam.AddEnumeration("Antenna Coordinate", AntennaCoordinate);
		enumParam.SetDefaultValue("0");
	}
	return true;
}
#endif

RADAR_LocInAntennaFrame::RADAR_LocInAntennaFrame()
{

}

bool RADAR_LocInAntennaFrame::Setup()
{
	TargetNum = TargetLoc.GetSize();
	return true;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool RADAR_LocInAntennaFrame::Run()
{
	SystemVueModelBuilder::Matrix<double>	tx(1, TargetNum), ty(1, TargetNum), tz(1, TargetNum);	// 声明各目标的绝对坐标
	SystemVueModelBuilder::Matrix<double>	dx(1, TargetNum), dy(1, TargetNum), dz(1, TargetNum);	// 声明各目标至雷达的相对坐标
	SystemVueModelBuilder::Matrix<double>	R(1, TargetNum);										// 声明各目标至雷达间的距离

	if (XYZFrameType == XYZFrame)
	{
		// 计算雷达在 XYZ 坐标系下的坐标
		double	rx = RadarLoc[0](0);
		double	ry = RadarLoc[0](1);
		double	rz = RadarLoc[0](2);

		for (int i = 0; i < TargetNum; i++)
		{	
			// 计算目标在 XYZ 坐标系下的坐标
			tx(i) = TargetLoc[i][0](0);
			ty(i) = TargetLoc[i][0](1);
			tz(i) = TargetLoc[i][0](2);

			// 计算各目标至雷达的相对坐标
			dx(i) = tx(i) - rx;
			dy(i) = ty(i) - ry;
			dz(i) = tz(i) - rz;

			// 计算各目标至雷达间的距离
			R(i) = sqrt(dx(i) * dx(i) + dy(i) * dy(i) + dz(i) * dz(i));	
		}
	}
	
	// LLA 坐标系已在 RADAR_Platform 与 RADAR_TargetScatterLocation 中转化为 ECI 坐标系
	else if (XYZFrameType == ECIFrame)
	{
		// 计算雷达在 ECI 坐标系下的坐标
		double	rx = RadarLoc[0](0);
		double	ry = RadarLoc[0](1);
		double	rz = RadarLoc[0](2);

		for (int i = 0; i < TargetNum; i++)
		{
			// 计算各目标在 ECI 坐标系下的坐标
			tx(i) = TargetLoc[i][0](0);
			ty(i) = TargetLoc[i][0](1);
			tz(i) = TargetLoc[i][0](2);

			// 计算各目标至雷达的相对坐标
			dx(i) = tx(i) - rx;
			dy(i) = ty(i) - ry;
			dz(i) = tz(i) - rz;

			// 计算各目标至雷达间的距离
			R(i) = sqrt(dx(i) * dx(i) + dy(i) * dy(i) + dz(i) * dz(i));
		}
	}

	///TODO///
	// 旋转变换
	if (BodyPitch.IsConnected())
	{
		double	BPitch = BodyPitch[0];
	}

	if (BodyRoll.IsConnected())
	{
		double BRoll = BodyRoll[0];
	}

	if (BodyYaw.IsConnected())
	{
		double BYaw = BodyYaw[0];
	}

	if (AntPitch.IsConnected())
	{
		double APitch = AntPitch[0];
	}

	if (AntRoll.IsConnected())
	{
		double ARoll = AntRoll[0];
	}

	if (AntYaw.IsConnected())
	{
		double AYaw = AntYaw[0];
	}

	// 计算方位角 Azimuth 与仰角 Elevation
	if (AntennaPlaneType == XYPlane)
	{
		for (int i = 0; i < TargetNum; i++)
		{
			Elevation[i][0] = asin(dz(i) / R(i));
			Azimuth[i][0] = atan2(dx(i), dz(i));
		}

	}

	else if (AntennaPlaneType == YZPlane)
	{
		for (int i = 0; i < TargetNum; i++)
		{
			Elevation[i][0] = asin(dz(i) / R(i));
			Azimuth[i][0] = atan2(dy(i), dx(i));
		}
	}
	
	return true;
}
