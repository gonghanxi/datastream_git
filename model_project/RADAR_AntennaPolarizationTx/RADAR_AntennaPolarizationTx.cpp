#include "RADAR_AntennaPolarizationTx.h"

#include <cmath>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include <cctype>
#include <cstdlib>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(RADAR_AntennaPolarizationTx)
{
	SET_MODEL_DESCRIPTION("Antenna Polarization for TX");
	SET_MODEL_SYMBOL("SYM_RADAR_AntennaPolarizationTx@RADAR Symbols");
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
		SystemVueModelBuilder::DFPort p = ADD_MODEL_INPUT(input);
		p.SetDescription("The input signal of antenna in the Tx chain");
	}

	// =========================
	// 输出端口
	// Port 6: output_V
	// Port 7: output_H
	// =========================
	{
		SystemVueModelBuilder::DFPort p = ADD_MODEL_OUTPUT(output_V);
		p.SetDescription("The vertical polarization output signal of antenna in the Tx chain");
	}

	{
		SystemVueModelBuilder::DFPort p = ADD_MODEL_OUTPUT(output_H);
		p.SetDescription("The horizontal polarization output signal of antenna in the Tx chain");
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
		p.SetDescription("Tx antenna pattern type defined by EMPro files, including 'UserDefine2D', 'UserDefine3D', read the user defined antenna pattern from file which is specified by the TxAntennaPatternFileName. UserDefine2D, UserDefine3D");
		p.SetDefaultValue("1");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(TxAntennaPatternFileName1);
		p.SetDescription("Tx antenna pattern filename");
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
		p.SetHideCondition("RadarWorkMode ~= 1 || AntennaScanPattern == 0");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAMETER(SectorScanEndAngle);
		p.SetDescription("The end angle of scan sector");
		p.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		p.SetDefaultValue("0");
		p.SetHideCondition("RadarWorkMode ~= 1 || AntennaScanPattern == 0");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAMETER(FlybackTime);
		p.SetDescription("The flyback time from the end position to start position");
		p.SetUnit(SystemVueModelBuilder::Units::TIME);
		p.SetDefaultValue("0");
		p.SetHideCondition("RadarWorkMode ~= 1 || (AntennaScanPattern ~= 2 && AntennaScanPattern ~= 4)");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAMETER(NumberOfRasterBars);
		p.SetDescription("The number of raster bars when using Bidirectional Raster or Unidirectional Raster scan");
		p.SetDefaultValue("0");
		p.SetHideCondition("RadarWorkMode ~= 1 || (AntennaScanPattern ~= 3 && AntennaScanPattern ~= 4)");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAMETER(RasterBarWidth);
		p.SetDescription("The angle between the raster bars when using Bidirectional Raster or Unidirectional Raster scan");
		p.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		p.SetDefaultValue("5");
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


RADAR_AntennaPolarizationTx::RADAR_AntennaPolarizationTx()
	: RadarWorkMode(Tracking)
	, ElementPatternFileType(EMPro)
	, ElementPatternFileScaleFactor(0)
	, ElementPatternFileScaleFactor_Size(0)
	, UserDefinedAntennaPattern(UserDefine3D)
	, TxAntennaPatternFileName1(0)
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


bool RADAR_AntennaPolarizationTx::Setup()
{
	// required envelope input
	input.SetRate(1);

	// ordinary real ports
	BeamAzimuth.SetRate(1);
	BeamElevation.SetRate(1);

	// bus input ports: SetRate only on subchannels
	for (size_t i = 0; i < TargetAzimuth.GetSize(); ++i)
	{
		TargetAzimuth[i].SetRate(1);
	}

	for (size_t i = 0; i < TargetElevation.GetSize(); ++i)
	{
		TargetElevation[i].SetRate(1);
	}

	// bus output ports: SetRate only on subchannels
	for (size_t i = 0; i < output_V.GetSize(); ++i)
	{
		output_V[i].SetRate(1);
	}

	for (size_t i = 0; i < output_H.GetSize(); ++i)
	{
		output_H[i].SetRate(1);
	}

	// 文件读取失败时不返回 false。
	// 这样可以先验证端口、参数、显隐、扫描逻辑。
	// 无文件时 lookupPolarizationGain() 使用单位方向图兜底。
	loadPatternFile();

	return true;
}


ERESULT RADAR_AntennaPolarizationTx::PropagateCharacterizationFrequency()
{
	double fc = 0.0;

	// input 是 EnvelopeCircularBuffer，Fc 应从 buffer 读取，不要写 input[0].Get...
	fc = input.GetCharacterizationFrequency();

	for (size_t i = 0; i < output_V.GetSize(); ++i)
	{
		output_V[i].SetCharacterizationFrequency(fc);
	}

	for (size_t i = 0; i < output_H.GetSize(); ++i)
	{
		output_H[i].SetCharacterizationFrequency(fc);
	}

	return static_cast<ERESULT>(0);
}


bool RADAR_AntennaPolarizationTx::Run()
{
	// EnvelopeSignal 不能直接赋值给 std::complex<double>，
	// 必须使用 complex() 取出复包络。
	const std::complex<double> x = input[0].complex();

	double timeNow = 0.0;
	timeNow = input.GetTime(0, GetCount());

	double beamAzRad = 0.0;
	double beamElRad = 0.0;
	getBeamAngle(timeNow, beamAzRad, beamElRad);

	const int targetCount = determineTargetCount();

	// 先清零所有已存在输出通道，避免部分通道没有被覆盖。
	for (size_t i = 0; i < output_V.GetSize(); ++i)
	{
		output_V[i][0] = std::complex<double>(0.0, 0.0);
	}

	for (size_t i = 0; i < output_H.GetSize(); ++i)
	{
		output_H[i][0] = std::complex<double>(0.0, 0.0);
	}

	const size_t outVSize = output_V.GetSize();
	const size_t outHSize = output_H.GetSize();

	const int maxWritable =
		static_cast<int>(std::max(outVSize, outHSize));

	const int nRun =
		(maxWritable > 0)
		? std::min(targetCount, maxWritable)
		: targetCount;

	for (int ch = 0; ch < nRun; ++ch)
	{
		double targetAzRad = 0.0;
		double targetElRad = 0.0;

		if (hasTargetAzimuthPort(ch))
		{
			targetAzRad = TargetAzimuth[static_cast<size_t>(ch)][0];
		}
		else
		{
			targetAzRad = deg2rad(getArrayValue(TargetAzimuthAngle,
				TargetAzimuthAngle_Size,
				ch,
				0.0));
		}

		if (hasTargetElevationPort(ch))
		{
			targetElRad = TargetElevation[static_cast<size_t>(ch)][0];
		}
		else
		{
			targetElRad = deg2rad(getArrayValue(TargetElevationAngle,
				TargetElevationAngle_Size,
				ch,
				0.0));
		}

		const double relAzRad = normalizeRad(targetAzRad - beamAzRad);
		const double relElRad = normalizeRad(targetElRad - beamElRad);

		double thetaDeg = 90.0;
		double phiDeg = 0.0;

		azelToPatternThetaPhi(relAzRad,
			relElRad,
			thetaDeg,
			phiDeg);

		std::complex<double> Gtheta(1.0, 0.0);
		std::complex<double> Gphi(1.0, 0.0);

		lookupPolarizationGain(thetaDeg,
			phiDeg,
			Gtheta,
			Gphi);

		// ElementPatternFileScaleFactor 默认 [1]。
		// 先按线性幅度缩放处理。
		// 如果后续黑盒发现是功率缩放，可改成 sqrt(scale)。
		const double scaleTheta = getScaleValue(0);
		const double scalePhi =
			(ElementPatternFileScaleFactor_Size >= 2)
			? getScaleValue(1)
			: scaleTheta;

		Gtheta *= scaleTheta;
		Gphi *= scalePhi;

		const std::complex<double> yV = x * Gtheta; // theta -> V
		const std::complex<double> yH = x * Gphi;   // phi   -> H

		if (static_cast<size_t>(ch) < output_V.GetSize())
		{
			output_V[static_cast<size_t>(ch)][0] = yV;
		}

		if (static_cast<size_t>(ch) < output_H.GetSize())
		{
			output_H[static_cast<size_t>(ch)][0] = yH;
		}
	}

	return true;
}


// ============================================================================
// 端口连接判断
// ============================================================================

bool RADAR_AntennaPolarizationTx::hasBeamAzimuthPort()
{
	return BeamAzimuth.IsConnected();
}


bool RADAR_AntennaPolarizationTx::hasBeamElevationPort()
{
	return BeamElevation.IsConnected();
}


bool RADAR_AntennaPolarizationTx::hasTargetAzimuthPort(int ch)
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


bool RADAR_AntennaPolarizationTx::hasTargetElevationPort(int ch)
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


int RADAR_AntennaPolarizationTx::determineTargetCount()
{
	int n = 0;

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

	// 输出 bus 的通道数量也作为目标数量的参考。
	n = std::max(n, static_cast<int>(output_V.GetSize()));
	n = std::max(n, static_cast<int>(output_H.GetSize()));

	if (n <= 0)
	{
		n = 1;
	}

	return n;
}


// ============================================================================
// 参数数组读取
// ============================================================================

double RADAR_AntennaPolarizationTx::getArrayValue(const double* data,
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


double RADAR_AntennaPolarizationTx::getScaleValue(int index) const
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

void RADAR_AntennaPolarizationTx::getBeamAngle(double timeNow,
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


double RADAR_AntennaPolarizationTx::getCircularScanAzimuth(double timeNow) const
{
	// ScanRate 单位 rpm，1 rpm = 6 deg/s
	const double rateDegPerSec = ScanRate * 6.0;

	if (rateDegPerSec == 0.0)
	{
		return 0.0;
	}

	return wrap360(rateDegPerSec * timeNow);
}


double RADAR_AntennaPolarizationTx::getSectorScanAzimuth(double timeNow,
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


void RADAR_AntennaPolarizationTx::getRasterScanAngle(double timeNow,
	bool bidirectional,
	double& azDeg,
	double& elDeg) const
{
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
// 方向图坐标转换
// ============================================================================

void RADAR_AntennaPolarizationTx::azelToPatternThetaPhi(double relAzRad,
	double relElRad,
	double& thetaDeg,
	double& phiDeg) const
{
	const double azDeg = rad2deg(relAzRad);
	const double elDeg = rad2deg(relElRad);

	// 帮助文档方向图为球坐标：
	// theta: 0~180
	// phi  : 0~360
	// az/el -> theta/phi 的常用转换为：
	// theta = 90 - elevation
	// phi   = azimuth wrapped to 0~360
	thetaDeg = clamp(90.0 - elDeg, 0.0, 180.0);
	phiDeg = wrap360(azDeg);
}


// ============================================================================
// 外部方向图文件读取
// ============================================================================

void RADAR_AntennaPolarizationTx::clearPattern()
{
	patternTable_.clear();
	patternLoaded_ = false;
	patternOpt_ = PatternFileOptions();
}


bool RADAR_AntennaPolarizationTx::loadPatternFile()
{
	clearPattern();

	if (TxAntennaPatternFileName1 == 0)
	{
		return false;
	}

	std::string fileName = TxAntennaPatternFileName1;

	if (fileName.empty())
	{
		return false;
	}

	std::ifstream fin(fileName.c_str());

	if (!fin.good())
	{
		return false;
	}

	bool inParameterSection = false;
	bool afterParameterSection = false;

	std::string line;

	while (std::getline(fin, line))
	{
		std::string t = trim(line);

		if (t.empty())
		{
			continue;
		}

		const std::string low = lowerString(t);

		if (low.find("begin_<parameters>") != std::string::npos ||
			low.find("begin_parameters") != std::string::npos)
		{
			inParameterSection = true;
			afterParameterSection = false;
			continue;
		}

		if (low.find("end_<parameters>") != std::string::npos ||
			low.find("end_parameters") != std::string::npos)
		{
			inParameterSection = false;
			afterParameterSection = true;
			continue;
		}

		if (inParameterSection)
		{
			parseParameterLine(t);
			continue;
		}

		if (!afterParameterSection)
		{
			// 有些文件可能没有显式 begin/end 参数段，
			// 这里跳过明显的文字说明行。
			std::vector<double> tryNums;
			if (!parseNumericLine(t.c_str(), tryNums))
			{
				continue;
			}
		}

		std::vector<double> nums;

		if (!parseNumericLine(t.c_str(), nums))
		{
			continue;
		}

		if (nums.size() < 4)
		{
			continue;
		}

		PatternPoint p;

		double theta = nums[0];
		double phi = nums[1];

		if (!patternOpt_.directionInDegrees)
		{
			theta = rad2deg(theta);
			phi = rad2deg(phi);
		}

		p.thetaDeg = theta;
		p.phiDeg = wrap360(phi);

		if (patternOpt_.useMagPhase)
		{
			// mag_phase:
			// theta_angle, phi_angle,
			// theta_gain, phi_gain,
			// theta_phase, phi_phase
			const double thetaGain = nums.size() > 2 ? nums[2] : 0.0;
			const double phiGain = nums.size() > 3 ? nums[3] : thetaGain;
			const double thetaPhase = nums.size() > 4 ? nums[4] : 0.0;
			const double phiPhase = nums.size() > 5 ? nums[5] : 0.0;

			p.Gtheta = magPhaseToComplex(thetaGain,
				thetaPhase,
				patternOpt_.magnitudeInDb,
				patternOpt_.phaseInDegrees);

			p.Gphi = magPhaseToComplex(phiGain,
				phiPhase,
				patternOpt_.magnitudeInDb,
				patternOpt_.phaseInDegrees);
		}
		else
		{
			// real_imag:
			// theta_angle, phi_angle,
			// real_theta_gain, imag_theta_gain,
			// real_phi_gain, imag_phi_gain
			const double thetaRe = nums.size() > 2 ? nums[2] : 1.0;
			const double thetaIm = nums.size() > 3 ? nums[3] : 0.0;
			const double phiRe = nums.size() > 4 ? nums[4] : thetaRe;
			const double phiIm = nums.size() > 5 ? nums[5] : thetaIm;

			p.Gtheta = std::complex<double>(thetaRe, thetaIm);
			p.Gphi = std::complex<double>(phiRe, phiIm);
		}

		patternTable_.push_back(p);
	}

	patternLoaded_ = !patternTable_.empty();

	return patternLoaded_;
}


bool RADAR_AntennaPolarizationTx::parseParameterLine(const std::string& line)
{
	std::string low = lowerString(line);

	// 去掉注释部分
	const size_t cpos = low.find("//");
	if (cpos != std::string::npos)
	{
		low = low.substr(0, cpos);
	}

	low = trim(low);

	if (low.empty())
	{
		return false;
	}

	if (low.find("mag_phase") != std::string::npos)
	{
		patternOpt_.useMagPhase = true;
		return true;
	}

	if (low.find("real_imag") != std::string::npos)
	{
		patternOpt_.useMagPhase = false;
		return true;
	}

	if (low.find("magnitude") != std::string::npos)
	{
		if (low.find("db") != std::string::npos)
		{
			patternOpt_.magnitudeInDb = true;
		}
		else if (low.find("linear") != std::string::npos)
		{
			patternOpt_.magnitudeInDb = false;
		}
		return true;
	}

	if (low.find("direction") != std::string::npos)
	{
		if (low.find("radian") != std::string::npos)
		{
			patternOpt_.directionInDegrees = false;
		}
		else if (low.find("degree") != std::string::npos)
		{
			patternOpt_.directionInDegrees = true;
		}
		return true;
	}

	if (low.find("phase") != std::string::npos)
	{
		if (low.find("radian") != std::string::npos)
		{
			patternOpt_.phaseInDegrees = false;
		}
		else if (low.find("degree") != std::string::npos)
		{
			patternOpt_.phaseInDegrees = true;
		}
		return true;
	}

	std::stringstream ss(low);
	std::string key;
	double value = 0.0;

	ss >> key >> value;

	if (!ss.fail())
	{
		if (key == "phi_min")
		{
			patternOpt_.phiMin = value;
			return true;
		}

		if (key == "phi_max")
		{
			patternOpt_.phiMax = value;
			return true;
		}

		if (key == "phi_inc")
		{
			patternOpt_.phiInc = value;
			return true;
		}

		if (key == "theta_min")
		{
			patternOpt_.thetaMin = value;
			return true;
		}

		if (key == "theta_max")
		{
			patternOpt_.thetaMax = value;
			return true;
		}

		if (key == "theta_inc")
		{
			patternOpt_.thetaInc = value;
			return true;
		}
	}

	return false;
}


bool RADAR_AntennaPolarizationTx::parseNumericLine(const char* line,
	std::vector<double>& nums) const
{
	nums.clear();

	if (line == 0)
	{
		return false;
	}

	std::string src = line;

	// 去掉 // 注释
	const size_t cpos = src.find("//");
	if (cpos != std::string::npos)
	{
		src = src.substr(0, cpos);
	}

	std::string cleaned;

	for (size_t i = 0; i < src.size(); ++i)
	{
		const char c = src[i];

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


void RADAR_AntennaPolarizationTx::lookupPolarizationGain(
	double thetaDeg,
	double phiDeg,
	std::complex<double>& Gtheta,
	std::complex<double>& Gphi) const
{
	// 无文件或文件解析失败时：
	// theta/V 与 phi/H 均给单位方向图，方便先做链路与端口验证。
	if (!patternLoaded_ || patternTable_.empty())
	{
		Gtheta = std::complex<double>(1.0, 0.0);
		Gphi = std::complex<double>(1.0, 0.0);
		return;
	}

	int bestIndex = 0;
	double bestScore = 1.0e300;

	for (size_t i = 0; i < patternTable_.size(); ++i)
	{
		const PatternPoint& p = patternTable_[i];

		const double dt = thetaDeg - p.thetaDeg;
		const double dp = angleDiffDeg(phiDeg, p.phiDeg);

		double score = 0.0;

		if (UserDefinedAntennaPattern == UserDefine2D)
		{
			// 2D 模式下主要按 phi/水平切面查找。
			score = dp * dp;
		}
		else
		{
			// 3D 模式按 theta/phi 二维最近邻查找。
			score = dt * dt + dp * dp;
		}

		if (score < bestScore)
		{
			bestScore = score;
			bestIndex = static_cast<int>(i);
		}
	}

	const PatternPoint& best = patternTable_[static_cast<size_t>(bestIndex)];

	Gtheta = best.Gtheta;
	Gphi = best.Gphi;
}


// ============================================================================
// 工具函数
// ============================================================================

std::string RADAR_AntennaPolarizationTx::trim(const std::string& s)
{
	size_t b = 0;
	while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b])))
	{
		++b;
	}

	size_t e = s.size();
	while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1])))
	{
		--e;
	}

	return s.substr(b, e - b);
}


std::string RADAR_AntennaPolarizationTx::lowerString(const std::string& s)
{
	std::string out = s;

	for (size_t i = 0; i < out.size(); ++i)
	{
		out[i] = static_cast<char>(
			std::tolower(static_cast<unsigned char>(out[i]))
			);
	}

	return out;
}


double RADAR_AntennaPolarizationTx::deg2rad(double x)
{
	return x * M_PI / 180.0;
}


double RADAR_AntennaPolarizationTx::rad2deg(double x)
{
	return x * 180.0 / M_PI;
}


double RADAR_AntennaPolarizationTx::normalizeRad(double x)
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


double RADAR_AntennaPolarizationTx::wrap360(double x)
{
	double y = std::fmod(x, 360.0);

	if (y < 0.0)
	{
		y += 360.0;
	}

	return y;
}


double RADAR_AntennaPolarizationTx::clamp(double x, double lo, double hi)
{
	if (x < lo)
	{
		return lo;
	}

	if (x > hi)
	{
		return hi;
	}

	return x;
}


double RADAR_AntennaPolarizationTx::angleDiffDeg(double a, double b)
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


std::complex<double> RADAR_AntennaPolarizationTx::magPhaseToComplex(
	double magnitude,
	double phase,
	bool magnitudeInDb,
	bool phaseInDegrees)
{
	const double amp =
		magnitudeInDb
		? std::pow(10.0, magnitude / 20.0)
		: magnitude;

	const double phaseRad =
		phaseInDegrees
		? deg2rad(phase)
		: phase;

	return std::complex<double>(amp * std::cos(phaseRad),
		amp * std::sin(phaseRad));
}