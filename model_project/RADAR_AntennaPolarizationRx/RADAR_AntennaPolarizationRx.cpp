#include "RADAR_AntennaPolarizationRx.h"

#include <cmath>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include <cctype>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(RADAR_AntennaPolarizationRx)
{
	SET_MODEL_DESCRIPTION("Antenna Polarization for RX");
	SET_MODEL_SYMBOL("SYM_RADAR_AntennaPolarizationRx@RADAR Symbols");
	SET_MODEL_CATEGORY("Signal Processing");

	// =========================
	// 输入端口
	// =========================
	{
		SystemVueModelBuilder::DFPort p = ADD_MODEL_INPUT(TargetAzimuth);
		p.SetDescription("The azimuth angle of target related to the radar reference coordinate (radian)");
		p.SetOptional(true);
	}

	{
		SystemVueModelBuilder::DFPort p = ADD_MODEL_INPUT(TargetElevation);
		p.SetDescription("The elevation angle of target related to the radar reference coordinate (radian)");
		p.SetOptional(true);
	}

	{
		SystemVueModelBuilder::DFPort p = ADD_MODEL_INPUT(BeamAzimuth);
		p.SetDescription("The azimuth angle of beam direction related to the radar reference coordinate (radian)");
		p.SetOptional(true);
	}

	{
		SystemVueModelBuilder::DFPort p = ADD_MODEL_INPUT(BeamElevation);
		p.SetDescription("The elevation angle of beam direction related to the radar reference coordinate (radian)");
		p.SetOptional(true);
	}

	{
		SystemVueModelBuilder::DFPort p = ADD_MODEL_INPUT(input_V);
		p.SetDescription("The vertical polarization input signal of antenna in the Rx chain");
		p.SetOptional(true);
	}

	{
		SystemVueModelBuilder::DFPort p = ADD_MODEL_INPUT(input_H);
		p.SetDescription("The horizontal polarization input signal of antenna in the Rx chain");
		p.SetOptional(true);
	}

	// =========================
	// 输出端口
	// =========================
	{
		SystemVueModelBuilder::DFPort p = ADD_MODEL_OUTPUT(output_V);
		p.SetDescription("The vertical polarization output signal of antenna in the Rx chain");
		p.SetOptional(true);
	}

	{
		SystemVueModelBuilder::DFPort p = ADD_MODEL_OUTPUT(output_H);
		p.SetDescription("The horizontal polarization output signal of antenna in the Rx chain");
		p.SetOptional(true);
	}

	// =========================
	// 参数
	// =========================
	{
		SystemVueModelBuilder::DFParam p =
			ADD_MODEL_ENUM_PARAMETER(RadarWorkMode, SelectedRadarWorkMode);

		p.AddEnumeration("Tracking", Tracking);
		p.AddEnumeration("Search", Search);
		p.SetDescription("Radar Work Mode : Tracking, Search");
		p.SetDefaultValue("0");
	}

	{
		SystemVueModelBuilder::DFParam p =
			ADD_MODEL_ENUM_PARAMETER(ElementPatternFileType, SelectedElementPatternFileType);

		p.AddEnumeration("EMPro", EMPro);
		p.AddEnumeration("HFSS", HFSS);
		p.AddEnumeration("CST", CST);
		p.SetDescription("The import antenna pattern file format: EMPro, HFSS, CST");
		p.SetDefaultValue("0");
	}

	{
		SystemVueModelBuilder::DFParam p =
			ADD_MODEL_ARRAY_PARAMETER(ElementPatternFileScaleFactor,
				ElementPatternFileScaleFactor_Size);

		p.SetDescription("Scale factor to scale element pattern file data to represent antenna gain");
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("[1]");
	}

	{
		SystemVueModelBuilder::DFParam p =
			ADD_MODEL_ENUM_PARAMETER(UserDefinedAntennaPattern,
				SelectedUserDefinedAntennaPattern);

		p.AddEnumeration("UserDefine2D", UserDefine2D);
		p.AddEnumeration("UserDefine3D", UserDefine3D);
		p.SetDescription("Rx antenna pattern type defined by EMPro files, including 'UserDefine2D', 'UserDefine3D', read the user defined antenna pattern from file which is specified by the RxAntennaPatternFileName1: UserDefine2D, UserDefine3D");
		p.SetDefaultValue("1");
	}

	{
		// 文件名参数：参考你给出的写法，使用 ADD_MODEL_PARAM + SetParamAsFile()
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(RxAntennaPatternFileName1);
		p.SetDescription("Rx antenna pattern filename");
		p.SetSchematicDisplay(0);
		p.SetParamAsFile();
	}

	{
		SystemVueModelBuilder::DFParam p =
			ADD_MODEL_ENUM_PARAMETER(AntennaScanPattern,
				SelectedAntennaScanPattern);

		p.AddEnumeration("Circular", CircularScan);
		p.AddEnumeration("Bidirectional Sector", BidirectionalSector);
		p.AddEnumeration("Unidirectional Sector", UnidirectionalSector);
		p.AddEnumeration("Bidirectional Raster", BidirectionalRaster);
		p.AddEnumeration("Unidirectional Raster", UnidirectionalRaster);

		p.SetDescription("Antenna Scan Pattern: Circular, Bidirectional Sector, Unidirectional Sector, Bidirectional Raster, Unidirectional Raster");
		p.SetDefaultValue("0");

		// Search 模式才显示
		p.SetHideCondition("RadarWorkMode ~= 1");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAMETER(ScanRate);
		p.SetDescription("Scan Rate sets the antenna scan rate, the unit is rpm(round per minute).");
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("15");
		p.SetHideCondition("RadarWorkMode ~= 1");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAMETER(ElevationAngle);
		p.SetDescription("Elevation Angle Value in degree");
		p.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		p.SetDefaultValue("0");
		p.SetHideCondition("RadarWorkMode ~= 1");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAMETER(SectorScanStartAngle);
		p.SetDescription("The start angle of scan sector");
		p.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		p.SetDefaultValue("0");

		// Sector / Raster 显示，Circular 隐藏
		p.SetHideCondition("RadarWorkMode ~= 1 || AntennaScanPattern == 0");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAMETER(SectorScanEndAngle);
		p.SetDescription("The end angle of scan sector");
		p.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		p.SetDefaultValue("0");

		// Sector / Raster 显示，Circular 隐藏
		p.SetHideCondition("RadarWorkMode ~= 1 || AntennaScanPattern == 0");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAMETER(FlybackTime);
		p.SetDescription("The flyback time from the end position to start position");
		p.SetUnit(SystemVueModelBuilder::Units::TIME);
		p.SetDefaultValue("0");

		// 仅 Unidirectional Sector / Unidirectional Raster 显示
		p.SetHideCondition("RadarWorkMode ~= 1 || (AntennaScanPattern ~= 2 && AntennaScanPattern ~= 4)");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAMETER(NumberOfRasterBars);
		p.SetDescription("The number of raster bars when using Bidirectional Raster or Unidirectional Raster scan");
		p.SetDefaultValue("0");

		// Raster 模式显示
		p.SetHideCondition("RadarWorkMode ~= 1 || (AntennaScanPattern ~= 3 && AntennaScanPattern ~= 4)");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAMETER(RasterBarWidth);
		p.SetDescription("The angle between the raster bars when using Bidirectional Raster or Unidirectional Raster scan");
		p.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		p.SetDefaultValue("5");

		// Raster 模式显示
		p.SetHideCondition("RadarWorkMode ~= 1 || (AntennaScanPattern ~= 3 && AntennaScanPattern ~= 4)");
	}

	{
		SystemVueModelBuilder::DFParam p =
			ADD_MODEL_ARRAY_PARAMETER(TargetAzimuthAngle,
				TargetAzimuthAngle_Size);

		p.SetDescription("The azimuth angle of target related to the radar reference coordinate");
		p.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		p.SetDefaultValue("[0]");
	}

	{
		SystemVueModelBuilder::DFParam p =
			ADD_MODEL_ARRAY_PARAMETER(TargetElevationAngle,
				TargetElevationAngle_Size);

		p.SetDescription("The elevation angle of target related to the radar reference coordinate");
		p.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		p.SetDefaultValue("[0]");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAMETER(BeamAzimuthAngle);
		p.SetDescription("The azimuth angle of beam direction related to the radar reference coordinate");
		p.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		p.SetDefaultValue("0");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAMETER(BeamElevationAngle);
		p.SetDescription("The elevation angle of beam direction related to the radar reference coordinate");
		p.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		p.SetDefaultValue("0");
	}

	return true;
}
#endif


RADAR_AntennaPolarizationRx::RADAR_AntennaPolarizationRx()
	: RadarWorkMode(Tracking)
	, ElementPatternFileType(EMPro)
	, ElementPatternFileScaleFactor(0)
	, ElementPatternFileScaleFactor_Size(0)
	, UserDefinedAntennaPattern(UserDefine3D)
	, RxAntennaPatternFileName1(0)
	, AntennaScanPattern(CircularScan)
	, ScanRate(15.0)
	, ElevationAngle(0.0)
	, SectorScanStartAngle(0.0)
	, SectorScanEndAngle(0.0)
	, FlybackTime(0.0)
	, NumberOfRasterBars(0)
	, RasterBarWidth(5.0)
	, TargetAzimuthAngle(0)
	, TargetAzimuthAngle_Size(0)
	, TargetElevationAngle(0)
	, TargetElevationAngle_Size(0)
	, BeamAzimuthAngle(0.0)
	, BeamElevationAngle(0.0)
	, patternLoaded_(false)
{
}


bool RADAR_AntennaPolarizationRx::Setup()
{
	// 普通 real 端口可以直接 SetRate
	BeamAzimuth.SetRate(1);
	BeamElevation.SetRate(1);

	// multiple real bus 只能对子通道 SetRate
	for (size_t i = 0; i < TargetAzimuth.GetSize(); ++i)
	{
		TargetAzimuth[i].SetRate(1);
	}

	for (size_t i = 0; i < TargetElevation.GetSize(); ++i)
	{
		TargetElevation[i].SetRate(1);
	}

	// multiple envelope bus 只能对子通道 SetRate
	for (size_t i = 0; i < input_H.GetSize(); ++i)
	{
		input_H[i].SetRate(1);
	}

	for (size_t i = 0; i < input_V.GetSize(); ++i)
	{
		input_V[i].SetRate(1);
	}

	// 输出是普通 envelope 端口
	output_H.SetRate(1);
	output_V.SetRate(1);

	// 读取外部方向图文件。
	// 如果文件为空或读取失败，不返回 false，使用默认单位极化矩阵，方便先完成端口/参数验证。
	loadPatternFile();

	return true;
}


ERESULT RADAR_AntennaPolarizationRx::PropagateCharacterizationFrequency()
{
	double fc = 0.0;

	for (size_t i = 0; i < input_H.GetSize(); ++i)
	{
		if (input_H[i].IsConnected())
		{
			fc = input_H[i].GetCharacterizationFrequency();
			break;
		}
	}

	if (fc <= 0.0)
	{
		for (size_t i = 0; i < input_V.GetSize(); ++i)
		{
			if (input_V[i].IsConnected())
			{
				fc = input_V[i].GetCharacterizationFrequency();
				break;
			}
		}
	}

	output_H.SetCharacterizationFrequency(fc);
	output_V.SetCharacterizationFrequency(fc);

	return static_cast<ERESULT>(0);
}


bool RADAR_AntennaPolarizationRx::Run()
{
	const int targetCount = determineTargetCount();

	std::complex<double> yH(0.0, 0.0);
	std::complex<double> yV(0.0, 0.0);

	double timeNow = 0.0;

	if (input_H.GetSize() > 0)
	{
		timeNow = input_H[0].GetTime(0, GetCount());
	}
	else if (input_V.GetSize() > 0)
	{
		timeNow = input_V[0].GetTime(0, GetCount());
	}

	double beamAzRad = 0.0;
	double beamElRad = 0.0;
	getBeamAngle(timeNow, beamAzRad, beamElRad);

	for (int ch = 0; ch < targetCount; ++ch)
	{
		std::complex<double> xH(0.0, 0.0);
		std::complex<double> xV(0.0, 0.0);

		if (hasInputHPort(ch))
		{
			xH = input_H[static_cast<size_t>(ch)][0].complex();
		}

		if (hasInputVPort(ch))
		{
			xV = input_V[static_cast<size_t>(ch)][0].complex();
		}

		double targetAzRad = 0.0;
		double targetElRad = 0.0;

		if (hasTargetAzimuthPort(ch))
		{
			targetAzRad = TargetAzimuth[static_cast<size_t>(ch)][0];
		}
		else
		{
			targetAzRad = deg2rad(getArrayValue(
				TargetAzimuthAngle,
				TargetAzimuthAngle_Size,
				ch,
				0.0
			));
		}

		if (hasTargetElevationPort(ch))
		{
			targetElRad = TargetElevation[static_cast<size_t>(ch)][0];
		}
		else
		{
			targetElRad = deg2rad(getArrayValue(
				TargetElevationAngle,
				TargetElevationAngle_Size,
				ch,
				0.0
			));
		}

		const double relAzDeg = rad2deg(normalizeRad(targetAzRad - beamAzRad));
		const double relElDeg = rad2deg(normalizeRad(targetElRad - beamElRad));

		std::complex<double> GHH;
		std::complex<double> GHV;
		std::complex<double> GVH;
		std::complex<double> GVV;

		lookupPolarizationMatrix(
			relAzDeg,
			relElDeg,
			GHH,
			GHV,
			GVH,
			GVV
		);

		yH += GHH * xH + GHV * xV;
		yV += GVH * xH + GVV * xV;
	}

	output_H[0] = yH;
	output_V[0] = yV;

	return true;
}


// ============================================================================
// 端口连接判断
// ============================================================================

bool RADAR_AntennaPolarizationRx::hasBeamAzimuthPort()
{
	return BeamAzimuth.IsConnected();
}


bool RADAR_AntennaPolarizationRx::hasBeamElevationPort()
{
	return BeamElevation.IsConnected();
}


bool RADAR_AntennaPolarizationRx::hasTargetAzimuthPort(int ch)
{
	if (ch < 0)
	{
		return false;
	}

	if (static_cast<size_t>(ch) >= TargetAzimuth.GetSize())
	{
		return false;
	}

	return TargetAzimuth[static_cast<size_t>(ch)].IsConnected();
}


bool RADAR_AntennaPolarizationRx::hasTargetElevationPort(int ch)
{
	if (ch < 0)
	{
		return false;
	}

	if (static_cast<size_t>(ch) >= TargetElevation.GetSize())
	{
		return false;
	}

	return TargetElevation[static_cast<size_t>(ch)].IsConnected();
}


bool RADAR_AntennaPolarizationRx::hasInputHPort(int ch)
{
	if (ch < 0)
	{
		return false;
	}

	if (static_cast<size_t>(ch) >= input_H.GetSize())
	{
		return false;
	}

	return input_H[static_cast<size_t>(ch)].IsConnected();
}


bool RADAR_AntennaPolarizationRx::hasInputVPort(int ch)
{
	if (ch < 0)
	{
		return false;
	}

	if (static_cast<size_t>(ch) >= input_V.GetSize())
	{
		return false;
	}

	return input_V[static_cast<size_t>(ch)].IsConnected();
}


int RADAR_AntennaPolarizationRx::determineTargetCount()
{
	int n = 0;

	for (size_t i = 0; i < input_H.GetSize(); ++i)
	{
		if (input_H[i].IsConnected())
		{
			n = std::max(n, static_cast<int>(i + 1));
		}
	}

	for (size_t i = 0; i < input_V.GetSize(); ++i)
	{
		if (input_V[i].IsConnected())
		{
			n = std::max(n, static_cast<int>(i + 1));
		}
	}

	for (size_t i = 0; i < TargetAzimuth.GetSize(); ++i)
	{
		if (TargetAzimuth[i].IsConnected())
		{
			n = std::max(n, static_cast<int>(i + 1));
		}
	}

	for (size_t i = 0; i < TargetElevation.GetSize(); ++i)
	{
		if (TargetElevation[i].IsConnected())
		{
			n = std::max(n, static_cast<int>(i + 1));
		}
	}

	n = std::max(n, TargetAzimuthAngle_Size);
	n = std::max(n, TargetElevationAngle_Size);

	if (n <= 0)
	{
		n = 1;
	}

	return n;
}


// ============================================================================
// 参数数组读取
// ============================================================================

double RADAR_AntennaPolarizationRx::getArrayValue(const double* data,
	int size,
	int index,
	double defaultValue) const
{
	if (data == 0 || size <= 0)
	{
		return defaultValue;
	}

	if (index < 0)
	{
		return data[0];
	}

	if (index < size)
	{
		return data[index];
	}

	return data[size - 1];
}


double RADAR_AntennaPolarizationRx::getScaleValue(int index) const
{
	if (ElementPatternFileScaleFactor == 0 ||
		ElementPatternFileScaleFactor_Size <= 0)
	{
		return 1.0;
	}

	if (index < 0)
	{
		return ElementPatternFileScaleFactor[0];
	}

	if (index < ElementPatternFileScaleFactor_Size)
	{
		return ElementPatternFileScaleFactor[index];
	}

	return ElementPatternFileScaleFactor[ElementPatternFileScaleFactor_Size - 1];
}


// ============================================================================
// Beam 角度计算
// ============================================================================

void RADAR_AntennaPolarizationRx::getBeamAngle(double timeNow,
	double& beamAzRad,
	double& beamElRad)
{
	if (hasBeamAzimuthPort())
	{
		beamAzRad = BeamAzimuth[0];
	}
	else
	{
		if (RadarWorkMode == Search)
		{
			if (AntennaScanPattern == CircularScan)
			{
				beamAzRad = deg2rad(getCircularScanAzimuth(timeNow));
			}
			else if (AntennaScanPattern == BidirectionalSector)
			{
				beamAzRad = deg2rad(getSectorScanAzimuth(timeNow, true));
			}
			else if (AntennaScanPattern == UnidirectionalSector)
			{
				beamAzRad = deg2rad(getSectorScanAzimuth(timeNow, false));
			}
			else if (AntennaScanPattern == BidirectionalRaster)
			{
				double azDeg = 0.0;
				double elDeg = 0.0;
				getRasterScanAngle(timeNow, true, azDeg, elDeg);
				beamAzRad = deg2rad(azDeg);
			}
			else if (AntennaScanPattern == UnidirectionalRaster)
			{
				double azDeg = 0.0;
				double elDeg = 0.0;
				getRasterScanAngle(timeNow, false, azDeg, elDeg);
				beamAzRad = deg2rad(azDeg);
			}
			else
			{
				beamAzRad = deg2rad(BeamAzimuthAngle);
			}
		}
		else
		{
			beamAzRad = deg2rad(BeamAzimuthAngle);
		}
	}

	if (hasBeamElevationPort())
	{
		beamElRad = BeamElevation[0];
	}
	else
	{
		if (RadarWorkMode == Search &&
			(AntennaScanPattern == BidirectionalRaster ||
				AntennaScanPattern == UnidirectionalRaster))
		{
			double azDeg = 0.0;
			double elDeg = 0.0;

			getRasterScanAngle(timeNow,
				AntennaScanPattern == BidirectionalRaster,
				azDeg,
				elDeg);

			beamElRad = deg2rad(elDeg);
		}
		else if (RadarWorkMode == Search)
		{
			beamElRad = deg2rad(ElevationAngle);
		}
		else
		{
			beamElRad = deg2rad(BeamElevationAngle);
		}
	}
}


double RADAR_AntennaPolarizationRx::getCircularScanAzimuth(double timeNow) const
{
	// ScanRate 单位 rpm，1 rpm = 6 deg/s
	const double rateDegPerSec = ScanRate * 6.0;

	if (rateDegPerSec == 0.0)
	{
		return 0.0;
	}

	return wrap360(rateDegPerSec * timeNow);
}


double RADAR_AntennaPolarizationRx::getSectorScanAzimuth(double timeNow,
	bool bidirectional) const
{
	const double startDeg = SectorScanStartAngle;
	const double endDeg = SectorScanEndAngle;

	double width = endDeg - startDeg;

	if (std::fabs(width) < 1.0e-15)
	{
		return startDeg;
	}

	const double dir = (width >= 0.0) ? 1.0 : -1.0;
	width = std::fabs(width);

	const double rateDegPerSec = std::fabs(ScanRate * 6.0);

	if (rateDegPerSec <= 0.0)
	{
		return startDeg;
	}

	const double forwardTime = width / rateDegPerSec;

	if (bidirectional)
	{
		const double period = 2.0 * forwardTime;

		if (period <= 0.0)
		{
			return startDeg;
		}

		double t = std::fmod(timeNow, period);

		if (t < 0.0)
		{
			t += period;
		}

		if (t <= forwardTime)
		{
			return startDeg + dir * rateDegPerSec * t;
		}

		return endDeg - dir * rateDegPerSec * (t - forwardTime);
	}
	else
	{
		const double fb = std::max(0.0, FlybackTime);
		const double period = forwardTime + fb;

		if (period <= 0.0)
		{
			return startDeg;
		}

		double t = std::fmod(timeNow, period);

		if (t < 0.0)
		{
			t += period;
		}

		if (t <= forwardTime)
		{
			return startDeg + dir * rateDegPerSec * t;
		}

		if (fb > 0.0)
		{
			const double k = (t - forwardTime) / fb;
			return endDeg + (startDeg - endDeg) * k;
		}

		return startDeg;
	}
}


void RADAR_AntennaPolarizationRx::getRasterScanAngle(double timeNow,
	bool bidirectional,
	double& azDeg,
	double& elDeg) const
{
	// 帮助文档没有 RetraceTime。
	// NumberOfRasterBars 默认 0，按 Bar No.0 起算，因此实际行数取 NumberOfRasterBars + 1。
	const int barCount = std::max(1, NumberOfRasterBars + 1);

	const double startDeg = SectorScanStartAngle;
	const double endDeg = SectorScanEndAngle;

	double width = endDeg - startDeg;

	if (std::fabs(width) < 1.0e-15)
	{
		azDeg = startDeg;
		elDeg = ElevationAngle;
		return;
	}

	const double dir = (width >= 0.0) ? 1.0 : -1.0;
	width = std::fabs(width);

	const double rateDegPerSec = std::fabs(ScanRate * 6.0);

	if (rateDegPerSec <= 0.0)
	{
		azDeg = startDeg;
		elDeg = ElevationAngle;
		return;
	}

	const double scanTime = width / rateDegPerSec;
	const double fb = std::max(0.0, FlybackTime);

	const double rowTime = bidirectional ? scanTime : (scanTime + fb);
	const double period = rowTime * barCount;

	if (period <= 0.0)
	{
		azDeg = startDeg;
		elDeg = ElevationAngle;
		return;
	}

	double t = std::fmod(timeNow, period);

	if (t < 0.0)
	{
		t += period;
	}

	int row = static_cast<int>(t / rowTime);

	if (row >= barCount)
	{
		row = barCount - 1;
	}

	const double rowLocal = t - row * rowTime;

	elDeg = ElevationAngle + row * RasterBarWidth;

	if (bidirectional)
	{
		const bool reverse = (row % 2) != 0;

		if (!reverse)
		{
			azDeg = startDeg + dir * rateDegPerSec * rowLocal;
		}
		else
		{
			azDeg = endDeg - dir * rateDegPerSec * rowLocal;
		}
	}
	else
	{
		if (rowLocal <= scanTime)
		{
			azDeg = startDeg + dir * rateDegPerSec * rowLocal;
		}
		else
		{
			if (fb > 0.0)
			{
				const double k = (rowLocal - scanTime) / fb;
				azDeg = endDeg + (startDeg - endDeg) * k;
			}
			else
			{
				azDeg = startDeg;
			}
		}
	}
}


// ============================================================================
// 外部方向图文件读取
// ============================================================================

void RADAR_AntennaPolarizationRx::clearPattern()
{
	patternTable_.clear();
	patternLoaded_ = false;
}


bool RADAR_AntennaPolarizationRx::loadPatternFile()
{
	clearPattern();

	if (RxAntennaPatternFileName1 == 0)
	{
		return false;
	}

	std::string fileName = RxAntennaPatternFileName1;

	if (fileName.empty())
	{
		return false;
	}

	std::ifstream fin(fileName.c_str());

	if (!fin.good())
	{
		return false;
	}

	std::string line;

	while (std::getline(fin, line))
	{
		std::vector<double> nums;

		if (!parseNumericLine(line.c_str(), nums))
		{
			continue;
		}

		if (nums.size() < 3)
		{
			continue;
		}

		PatternPoint p;
		p.azDeg = nums[0];
		p.elDeg = (nums.size() >= 4) ? nums[1] : 0.0;

		p.GHH = std::complex<double>(1.0, 0.0);
		p.GHV = std::complex<double>(0.0, 0.0);
		p.GVH = std::complex<double>(0.0, 0.0);
		p.GVV = std::complex<double>(1.0, 0.0);

		if (nums.size() >= 10)
		{
			// 通用复数矩阵格式：
			// az el GHH_re GHH_im GHV_re GHV_im GVH_re GVH_im GVV_re GVV_im
			p.GHH = std::complex<double>(nums[2], nums[3]);
			p.GHV = std::complex<double>(nums[4], nums[5]);
			p.GVH = std::complex<double>(nums[6], nums[7]);
			p.GVV = std::complex<double>(nums[8], nums[9]);
		}
		else if (nums.size() >= 6)
		{
			// 简化 dB + phase 格式：
			// az el H_dB H_phaseDeg V_dB V_phaseDeg
			p.GHH = dbPhaseToComplex(nums[2], nums[3]);
			p.GVV = dbPhaseToComplex(nums[4], nums[5]);
			p.GHV = std::complex<double>(0.0, 0.0);
			p.GVH = std::complex<double>(0.0, 0.0);
		}
		else if (nums.size() >= 4)
		{
			// 简化 dB 格式：
			// az el H_dB V_dB
			p.GHH = dbPhaseToComplex(nums[2], 0.0);
			p.GVV = dbPhaseToComplex(nums[3], 0.0);
			p.GHV = std::complex<double>(0.0, 0.0);
			p.GVH = std::complex<double>(0.0, 0.0);
		}
		else if (nums.size() >= 3)
		{
			// 单通道 dB 格式：
			// az el gain_dB
			p.GHH = dbPhaseToComplex(nums[2], 0.0);
			p.GVV = p.GHH;
			p.GHV = std::complex<double>(0.0, 0.0);
			p.GVH = std::complex<double>(0.0, 0.0);
		}

		patternTable_.push_back(p);
	}

	patternLoaded_ = !patternTable_.empty();

	return patternLoaded_;
}


bool RADAR_AntennaPolarizationRx::parseNumericLine(const char* line,
	std::vector<double>& nums) const
{
	nums.clear();

	if (line == 0)
	{
		return false;
	}

	std::string cleaned;

	for (const char* p = line; *p != '\0'; ++p)
	{
		const char c = *p;

		if ((c >= '0' && c <= '9') ||
			c == '.' ||
			c == '-' ||
			c == '+' ||
			c == 'e' ||
			c == 'E')
		{
			cleaned.push_back(c);
		}
		else
		{
			cleaned.push_back(' ');
		}
	}

	std::stringstream ss(cleaned);

	double v = 0.0;

	while (ss >> v)
	{
		nums.push_back(v);
	}

	return !nums.empty();
}


void RADAR_AntennaPolarizationRx::lookupPolarizationMatrix(
	double relAzDeg,
	double relElDeg,
	std::complex<double>& GHH,
	std::complex<double>& GHV,
	std::complex<double>& GVH,
	std::complex<double>& GVV) const
{
	// 没有文件或文件未解析成功时，默认单位极化矩阵：
	// input_H -> output_H
	// input_V -> output_V
	if (!patternLoaded_ || patternTable_.empty())
	{
		GHH = std::complex<double>(1.0, 0.0);
		GHV = std::complex<double>(0.0, 0.0);
		GVH = std::complex<double>(0.0, 0.0);
		GVV = std::complex<double>(1.0, 0.0);

		return;
	}

	int bestIndex = 0;
	double bestScore = 1.0e300;

	for (size_t i = 0; i < patternTable_.size(); ++i)
	{
		const PatternPoint& p = patternTable_[i];

		const double da = angleDiffDeg(relAzDeg, p.azDeg);

		double de = 0.0;

		if (UserDefinedAntennaPattern == UserDefine3D)
		{
			de = relElDeg - p.elDeg;
		}

		const double score = da * da + de * de;

		if (score < bestScore)
		{
			bestScore = score;
			bestIndex = static_cast<int>(i);
		}
	}

	const PatternPoint& p = patternTable_[static_cast<size_t>(bestIndex)];

	GHH = p.GHH;
	GHV = p.GHV;
	GVH = p.GVH;
	GVV = p.GVV;

	// ScaleFactor 作为幅度比例直接作用。
	// [1] 时四个通道都乘 1。
	// 如果给出 4 个数，则分别作用到 GHH/GHV/GVH/GVV。
	const double s0 = getScaleValue(0);
	const double s1 = getScaleValue(1);
	const double s2 = getScaleValue(2);
	const double s3 = getScaleValue(3);

	if (ElementPatternFileScaleFactor_Size >= 4)
	{
		GHH *= s0;
		GHV *= s1;
		GVH *= s2;
		GVV *= s3;
	}
	else
	{
		GHH *= s0;
		GHV *= s0;
		GVH *= s0;
		GVV *= s0;
	}
}


// ============================================================================
// 工具函数
// ============================================================================

double RADAR_AntennaPolarizationRx::deg2rad(double x)
{
	return x * M_PI / 180.0;
}


double RADAR_AntennaPolarizationRx::rad2deg(double x)
{
	return x * 180.0 / M_PI;
}


double RADAR_AntennaPolarizationRx::normalizeRad(double x)
{
	while (x > M_PI)
	{
		x -= 2.0 * M_PI;
	}

	while (x < -M_PI)
	{
		x += 2.0 * M_PI;
	}

	return x;
}


double RADAR_AntennaPolarizationRx::wrap360(double x)
{
	double y = std::fmod(x, 360.0);

	if (y < 0.0)
	{
		y += 360.0;
	}

	return y;
}


double RADAR_AntennaPolarizationRx::angleDiffDeg(double a, double b)
{
	double d = a - b;

	while (d > 180.0)
	{
		d -= 360.0;
	}

	while (d < -180.0)
	{
		d += 360.0;
	}

	return d;
}


std::complex<double> RADAR_AntennaPolarizationRx::dbPhaseToComplex(double db,
	double phaseDeg)
{
	const double amp = std::pow(10.0, db / 20.0);
	const double ph = deg2rad(phaseDeg);

	return std::complex<double>(amp * std::cos(ph),
		amp * std::sin(ph));
}