#include "RADAR_Antenna_Rx.h"

#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.141592653589793238462643383279502884
#endif

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(RADAR_Antenna_Rx)
{
	SET_MODEL_DESCRIPTION("Receiving Antenna");
	SET_MODEL_SYMBOL("SYM_RADAR_Antenna_Rx@RADAR Symbols");
	SET_MODEL_CATEGORY("Signal Processing");

	// ============================================================
	// Ports
	// ============================================================

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(TargetAzimuth);
		port.SetDescription("The azimuth angle of target related to the radar reference coordinate (radian)");
		port.SetOptional(true);
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(TargetElevation);
		port.SetDescription("The elevation angle of target related to the radar reference coordinate (radian)");
		port.SetOptional(true);
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(BeamAzimuth);
		port.SetDescription("The azimuth angle of beam direction related to the radar reference coordinate (radian)");
		port.SetOptional(true);
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(BeamElevation);
		port.SetDescription("The elevation angle of beam direction related to the radar reference coordinate (radian)");
		port.SetOptional(true);
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(input);
		port.SetDescription("The input signal of antenna in the Rx chain");
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
		port.SetDescription("The output signal of antenna in the Rx chain");
	}

	// ============================================================
	// RadarWorkMode
	// ============================================================

	{
		SystemVueModelBuilder::DFParam enumParam =
			ADD_MODEL_ENUM_PARAM(RadarWorkMode, SelectedRadarWorkMode);

		enumParam.SetDescription("Radar Work Mode : Tracking, Search");
		enumParam.AddEnumeration("Tracking", Tracking);
		enumParam.AddEnumeration("Search", Search);
		enumParam.SetDefaultValue("0");
	}

	// ============================================================
	// Pattern
	// ============================================================

	{
		SystemVueModelBuilder::DFParam enumParam =
			ADD_MODEL_ENUM_PARAM(Pattern, SelectedPattern);

		enumParam.SetDescription(
			"User-defined antenna pattern or antenna pattern which is decided by illumination distribution function: "
			"UserDefinedPattern, Uniform, Cosine, Parabolic, Triangle, Circular, CosineSquaredPedestal, Taylor"
		);

		enumParam.AddEnumeration("UserDefinedPattern", UserDefinedPattern);
		enumParam.AddEnumeration("Uniform", Uniform);
		enumParam.AddEnumeration("Cosine", Cosine);
		enumParam.AddEnumeration("Parabolic", Parabolic);
		enumParam.AddEnumeration("Triangle", Triangle);
		enumParam.AddEnumeration("Circular", Circular);
		enumParam.AddEnumeration("CosineSquaredPedestal", CosineSquaredPedestal);
		enumParam.AddEnumeration("Taylor", Taylor);
		enumParam.SetDefaultValue("1");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(Factor1);
		param.SetDescription("The factor which is defined in the distribution function");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("0");
		param.SetHideCondition("Pattern == 0 || Pattern == 1");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(Factor2);
		param.SetDescription("The factor which is defined in the distribution function");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("0");
		param.SetHideCondition("Pattern ~= 6");
	}

	{
		SystemVueModelBuilder::DFParam param =
			ADD_MODEL_ARRAY_PARAM(AntennaPatternArray, AntennaPatternArray_Size);

		param.SetDescription("Antenna Pattern: elevation * azimuth with dB");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("[ones(180 * 360, 1)]");
		param.SetHideCondition("Pattern ~= 0");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(Sidelobe_Levels);
		param.SetDescription("Sidelobe_levels in dB");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("-20");
		param.SetHideCondition("Pattern ~= 7");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(nBar);
		param.SetDescription("This parameter is used to generate the Taylor distribution");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("2");
		param.SetHideCondition("Pattern ~= 7");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(AntennaHeight);
		param.SetDescription("The vertical length of antenna");
		param.SetUnit(SystemVueModelBuilder::Units::LENGTH);
		param.SetDefaultValue("5");
		param.SetHideCondition("Pattern == 0");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(AntennaWidth);
		param.SetDescription("The horizontal length of antenna");
		param.SetUnit(SystemVueModelBuilder::Units::LENGTH);
		param.SetDefaultValue("5");
		param.SetHideCondition("Pattern == 0");
	}

	{
		SystemVueModelBuilder::DFParam enumParam =
			ADD_MODEL_ENUM_PARAM(AntennaScanPattern, SelectedAntennaScanPattern);

		enumParam.SetDescription(
			"Antenna Scan Pattern: Circular, Bidirectional Sector, Unidirectional Sector, "
			"Bidirectional Raster, Unidirectional Raster"
		);

		enumParam.AddEnumeration("Circular", CircularScan);
		enumParam.AddEnumeration("Bidirectional Sector", BidirectionalSector);
		enumParam.AddEnumeration("Unidirectional Sector", UnidirectionalSector);
		enumParam.AddEnumeration("Bidirectional Raster", BidirectionalRaster);
		enumParam.AddEnumeration("Unidirectional Raster", UnidirectionalRaster);
		enumParam.SetDefaultValue("0");
		enumParam.SetHideCondition("RadarWorkMode ~= 1");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(ScanRate);
		param.SetDescription("Scan Rate sets the antenna scan rate, the unit is rpm(round per minute).");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("15");
		param.SetHideCondition("RadarWorkMode ~= 1");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(ElevationAngle);
		param.SetDescription("Elevation Angle Value in degree");
		param.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		param.SetDefaultValue("0");
		param.SetHideCondition("RadarWorkMode ~= 1");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(SectorScanStartAngle);
		param.SetDescription("The start angle of scan sector");
		param.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		param.SetDefaultValue("0");
		param.SetHideCondition("RadarWorkMode ~= 1 || AntennaScanPattern == 0");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(SectorScanEndAngle);
		param.SetDescription("The end angle of scan sector");
		param.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		param.SetDefaultValue("0");
		param.SetHideCondition("RadarWorkMode ~= 1 || AntennaScanPattern == 0");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(FlybackTime);
		param.SetDescription("The flyback time from the end position to start position");
		param.SetUnit(SystemVueModelBuilder::Units::TIME);
		param.SetDefaultValue("0");
		param.SetHideCondition(
			"RadarWorkMode ~= 1 || AntennaScanPattern == 0 || AntennaScanPattern == 1 || AntennaScanPattern == 3"
		);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(NumberOfRasterBars);
		param.SetDescription("The number of raster bars when using Bidirectional Raster or Unidirectional Raster scan");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("0");
		param.SetHideCondition(
			"RadarWorkMode ~= 1 || AntennaScanPattern == 0 || AntennaScanPattern == 1 || AntennaScanPattern == 2"
		);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(RasterBarWidth);
		param.SetDescription("The angle between the raster bars when using Bidirectional Raster or Unidirectional Raster scan");
		param.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		param.SetDefaultValue("5");
		param.SetHideCondition(
			"RadarWorkMode ~= 1 || AntennaScanPattern == 0 || AntennaScanPattern == 1 || AntennaScanPattern == 2"
		);
	}

	{
		SystemVueModelBuilder::DFParam param =
			ADD_MODEL_ARRAY_PARAM(TargetAzimuthAngle, TargetAzimuthAngle_Size);

		param.SetDescription("The azimuth angle of target related to the radar reference coordinate");
		param.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		param.SetDefaultValue("[0]");
	}

	{
		SystemVueModelBuilder::DFParam param =
			ADD_MODEL_ARRAY_PARAM(TargetElevationAngle, TargetElevationAngle_Size);

		param.SetDescription("The elevation angle of target related to the radar reference coordinate");
		param.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		param.SetDefaultValue("[0]");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(BeamAzimuthAngle);
		param.SetDescription("The azimuth angle of beam direction related to the radar reference coordinate");
		param.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		param.SetDefaultValue("0");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(BeamElevationAngle);
		param.SetDescription("The elevation angle of beam direction related to the radar reference coordinate");
		param.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		param.SetDefaultValue("0");
	}

	return true;
}
#endif


RADAR_Antenna_Rx::RADAR_Antenna_Rx()
	: RadarWorkMode(Tracking)
	, Pattern(Uniform)
	, Factor1(0.0)
	, Factor2(0.0)
	, AntennaPatternArray(0)
	, AntennaPatternArray_Size(0)
	, Sidelobe_Levels(-20.0)
	, nBar(2)
	, AntennaHeight(5.0)
	, AntennaWidth(5.0)
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
{
}


bool RADAR_Antenna_Rx::Setup()
{
	for (size_t i = 0; i < TargetAzimuth.GetSize(); ++i)
	{
		TargetAzimuth[i].SetRate(1);
	}

	for (size_t i = 0; i < TargetElevation.GetSize(); ++i)
	{
		TargetElevation[i].SetRate(1);
	}

	BeamAzimuth.SetRate(1);
	BeamElevation.SetRate(1);

	for (size_t i = 0; i < input.GetSize(); ++i)
	{
		input[i].SetRate(1);
	}

	output.SetRate(1);

	return true;
}


ERESULT RADAR_Antenna_Rx::PropagateCharacterizationFrequency()
{
	if (input.GetSize() > 0)
	{
		output.SetCharacterizationFrequency(input[0].GetCharacterizationFrequency());
	}
	else
	{
		output.SetCharacterizationFrequency(0.0);
	}

	return true;
}


bool RADAR_Antenna_Rx::Run()
{
	const int nTarget = static_cast<int>(input.GetSize());

	if (nTarget <= 0)
	{
		output[0] = std::complex<double>(0.0, 0.0);
		return true;
	}

	double timeNow = 0.0;

	if (input[0].GetSize() > 0)
	{
		timeNow = input[0].GetTime(0, GetCount());
	}

	const double fcHz = input[0].GetCharacterizationFrequency();

	double beamAz = 0.0;
	double beamEl = 0.0;
	getBeamAngle(timeNow, beamAz, beamEl);

	std::complex<double> y(0.0, 0.0);

	const double apertureGain = calcApertureGainLinear(fcHz);

	for (int ch = 0; ch < nTarget; ++ch)
	{
		const double targetAz = hasTargetAzimuthPort(ch)
			? TargetAzimuth[ch][0]
			: deg2rad(getArrayValue(TargetAzimuthAngle,
				TargetAzimuthAngle_Size,
				ch,
				0.0));

		const double targetEl = hasTargetElevationPort(ch)
			? TargetElevation[ch][0]
			: deg2rad(getArrayValue(TargetElevationAngle,
				TargetElevationAngle_Size,
				ch,
				0.0));

		const double sep = angularSeparation(targetAz, targetEl, beamAz, beamEl);

		if (sep > 0.5 * M_PI)
		{
			continue;
		}

		const double patternGainDb = calcPatternGainDb(targetAz,
			targetEl,
			beamAz,
			beamEl,
			fcHz);

		const double patternGain = std::pow(10.0, patternGainDb / 20.0);

		const std::complex<double> xin = input[ch][0].complex();

		y += xin * apertureGain * patternGain;
	}

	output[0] = y;

	return true;
}


// ============================================================================
// Basic tools
// ============================================================================

double RADAR_Antenna_Rx::deg2rad(double x)
{
	return x * M_PI / 180.0;
}


double RADAR_Antenna_Rx::rad2deg(double x)
{
	return x * 180.0 / M_PI;
}


double RADAR_Antenna_Rx::wrapToPi(double x)
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


double RADAR_Antenna_Rx::wrapTo360(double x)
{
	double y = std::fmod(x, 360.0);

	if (y < 0.0)
	{
		y += 360.0;
	}

	return y;
}


double RADAR_Antenna_Rx::clampValue(double v, double lo, double hi)
{
	if (v < lo)
	{
		return lo;
	}

	if (v > hi)
	{
		return hi;
	}

	return v;
}


double RADAR_Antenna_Rx::sinc(double x)
{
	if (std::fabs(x) < 1.0e-12)
	{
		return 1.0;
	}

	return std::sin(x) / x;
}


double RADAR_Antenna_Rx::besselI0(double x)
{
	double sum = 1.0;
	double term = 1.0;

	const double xx = 0.25 * x * x;

	for (int k = 1; k < 40; ++k)
	{
		term *= xx / static_cast<double>(k * k);
		sum += term;

		if (std::fabs(term) < 1.0e-14 * std::fabs(sum))
		{
			break;
		}
	}

	return sum;
}


// ============================================================================
// Port connection helpers
// ============================================================================

bool RADAR_Antenna_Rx::hasBeamAzimuthPort()
{
	return BeamAzimuth.IsConnected();
}


bool RADAR_Antenna_Rx::hasBeamElevationPort()
{
	return BeamElevation.IsConnected();
}


bool RADAR_Antenna_Rx::hasTargetAzimuthPort(int ch)
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


bool RADAR_Antenna_Rx::hasTargetElevationPort(int ch)
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


double RADAR_Antenna_Rx::getArrayValue(const double* data,
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


// ============================================================================
// Beam angle
// ============================================================================

void RADAR_Antenna_Rx::getBeamAngle(double timeNow,
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
				beamElRad = deg2rad(elDeg);
			}
			else if (AntennaScanPattern == UnidirectionalRaster)
			{
				double azDeg = 0.0;
				double elDeg = 0.0;

				getRasterScanAngle(timeNow, false, azDeg, elDeg);

				beamAzRad = deg2rad(azDeg);
				beamElRad = deg2rad(elDeg);
			}
			else
			{
				beamAzRad = 0.0;
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
		if (RadarWorkMode == Search)
		{
			if (AntennaScanPattern == BidirectionalRaster ||
				AntennaScanPattern == UnidirectionalRaster)
			{
				double azDeg = 0.0;
				double elDeg = 0.0;

				getRasterScanAngle(timeNow,
					AntennaScanPattern == BidirectionalRaster,
					azDeg,
					elDeg);

				beamElRad = deg2rad(elDeg);
			}
			else
			{
				beamElRad = deg2rad(ElevationAngle);
			}
		}
		else
		{
			beamElRad = deg2rad(BeamElevationAngle);
		}
	}
}


double RADAR_Antenna_Rx::getCircularScanAzimuth(double timeNow) const
{
	const double rateDegPerSec = ScanRate * 6.0;
	return wrapTo360(rateDegPerSec * timeNow);
}


double RADAR_Antenna_Rx::getSectorScanAzimuth(double timeNow,
	bool bidirectional) const
{
	const double startDeg = SectorScanStartAngle;
	const double endDeg = SectorScanEndAngle;
	const double span = std::fabs(endDeg - startDeg);
	const double dir = (endDeg >= startDeg) ? 1.0 : -1.0;
	const double rate = std::fabs(ScanRate * 6.0);

	if (span <= 1.0e-12 || rate <= 1.0e-12)
	{
		return startDeg;
	}

	const double oneWayTime = span / rate;

	if (bidirectional)
	{
		const double period = 2.0 * oneWayTime;

		double t = std::fmod(timeNow, period);
		if (t < 0.0)
		{
			t += period;
		}

		if (t <= oneWayTime)
		{
			return startDeg + dir * rate * t;
		}

		return endDeg - dir * rate * (t - oneWayTime);
	}
	else
	{
		const double fb = (FlybackTime > 0.0) ? FlybackTime : 0.0;
		const double period = oneWayTime + fb;

		double t = std::fmod(timeNow, period);
		if (t < 0.0)
		{
			t += period;
		}

		if (t <= oneWayTime)
		{
			return startDeg + dir * rate * t;
		}

		return startDeg;
	}
}


void RADAR_Antenna_Rx::getRasterScanAngle(double timeNow,
	bool bidirectional,
	double& azDeg,
	double& elDeg) const
{
	const int bars = (NumberOfRasterBars > 0) ? NumberOfRasterBars : 1;

	const double startDeg = SectorScanStartAngle;
	const double endDeg = SectorScanEndAngle;
	const double span = std::fabs(endDeg - startDeg);
	const double dir = (endDeg >= startDeg) ? 1.0 : -1.0;
	const double rate = std::fabs(ScanRate * 6.0);

	if (span <= 1.0e-12 || rate <= 1.0e-12)
	{
		azDeg = startDeg;
		elDeg = ElevationAngle;
		return;
	}

	const double oneBarTime = span / rate;

	if (bidirectional)
	{
		const double totalTime = oneBarTime * static_cast<double>(bars);

		double t = std::fmod(timeNow, totalTime);
		if (t < 0.0)
		{
			t += totalTime;
		}

		int bar = static_cast<int>(std::floor(t / oneBarTime));

		if (bar < 0)
		{
			bar = 0;
		}

		if (bar >= bars)
		{
			bar = bars - 1;
		}

		const double localT = t - static_cast<double>(bar) * oneBarTime;
		const bool reverse = (bar % 2) != 0;

		if (!reverse)
		{
			azDeg = startDeg + dir * rate * localT;
		}
		else
		{
			azDeg = endDeg - dir * rate * localT;
		}

		elDeg = ElevationAngle + static_cast<double>(bar) * RasterBarWidth;
	}
	else
	{
		const double fb = (FlybackTime > 0.0) ? FlybackTime : 0.0;
		const double oneCycle = oneBarTime + fb;
		const double totalTime = oneCycle * static_cast<double>(bars);

		double t = std::fmod(timeNow, totalTime);
		if (t < 0.0)
		{
			t += totalTime;
		}

		int bar = static_cast<int>(std::floor(t / oneCycle));

		if (bar < 0)
		{
			bar = 0;
		}

		if (bar >= bars)
		{
			bar = bars - 1;
		}

		const double localT = t - static_cast<double>(bar) * oneCycle;

		if (localT <= oneBarTime)
		{
			azDeg = startDeg + dir * rate * localT;
		}
		else
		{
			azDeg = startDeg;
		}

		elDeg = ElevationAngle + static_cast<double>(bar) * RasterBarWidth;
	}
}


// ============================================================================
// Gain / pattern
// ============================================================================

double RADAR_Antenna_Rx::angularSeparation(double az1,
	double el1,
	double az2,
	double el2) const
{
	const double c1 = std::cos(el1);
	const double c2 = std::cos(el2);

	const double x1 = c1 * std::cos(az1);
	const double y1 = c1 * std::sin(az1);
	const double z1 = std::sin(el1);

	const double x2 = c2 * std::cos(az2);
	const double y2 = c2 * std::sin(az2);
	const double z2 = std::sin(el2);

	const double dot = clampValue(x1 * x2 + y1 * y2 + z1 * z2, -1.0, 1.0);

	return std::acos(dot);
}


double RADAR_Antenna_Rx::calcPatternGainDb(double targetAzRad,
	double targetElRad,
	double beamAzRad,
	double beamElRad,
	double fcHz) const
{
	const double dAz = wrapToPi(targetAzRad - beamAzRad);
	const double dEl = targetElRad - beamElRad;

	if (Pattern == UserDefinedPattern)
	{
		return calcUserPatternGainDb(dAz, dEl);
	}

	return calcAnalyticPatternGainDb(dAz, dEl, fcHz);
}


double RADAR_Antenna_Rx::calcUserPatternGainDb(double dAzRad,
	double dElRad) const
{
	if (AntennaPatternArray == 0 || AntennaPatternArray_Size <= 0)
	{
		return 0.0;
	}

	const double azDeg = wrapTo360(rad2deg(dAzRad) + 180.0);
	const double elDeg = clampValue(rad2deg(dElRad) + 90.0, 0.0, 179.999999);

	const int col = static_cast<int>(std::floor(azDeg));
	const int row = static_cast<int>(std::floor(elDeg));
	const int idx = row * 360 + col;

	if (idx >= 0 && idx < AntennaPatternArray_Size)
	{
		return AntennaPatternArray[idx];
	}

	const int safeIdx =
		static_cast<int>(clampValue(static_cast<double>(idx),
			0.0,
			static_cast<double>(AntennaPatternArray_Size - 1)));

	return AntennaPatternArray[safeIdx];
}


double RADAR_Antenna_Rx::calcAnalyticPatternGainDb(double dAzRad,
	double dElRad,
	double fcHz) const
{
	// 方向图形状使用归一化口径响应；绝对孔径增益由 calcApertureGainLinear 单独乘入。
	const double c0 = 3.0e8;

	double lambda = 1.0;
	if (fcHz > 1.0)
	{
		lambda = c0 / fcHz;
	}

	const double width = (AntennaWidth > 1.0e-12) ? AntennaWidth : 1.0e-12;
	const double height = (AntennaHeight > 1.0e-12) ? AntennaHeight : 1.0e-12;

	const double ux = M_PI * width / lambda * std::sin(dAzRad);
	const double uy = M_PI * height / lambda * std::sin(dElRad);

	double amp = std::fabs(sinc(ux) * sinc(uy));

	const double azNorm = clampValue(std::fabs(dAzRad) / (0.5 * M_PI), 0.0, 1.0);
	const double elNorm = clampValue(std::fabs(dElRad) / (0.5 * M_PI), 0.0, 1.0);
	const double uNorm = clampValue(std::sqrt(azNorm * azNorm + elNorm * elNorm), 0.0, 1.0);

	amp *= calcDistributionWeight(uNorm);

	if (amp < 1.0e-300)
	{
		amp = 1.0e-300;
	}

	return 20.0 * std::log10(amp);
}


double RADAR_Antenna_Rx::calcApertureGainLinear(double fcHz) const
{
	if (Pattern == UserDefinedPattern)
	{
		return 1.0;
	}

	if (fcHz <= 1.0)
	{
		return 1.0;
	}

	// 黑盒校准：
	// Tracking + Uniform + target/beam 正对 + H=W=5m + Fc=10GHz 时，
	// 内置输出为 98469.658。
	// 该结果对应：
	//     sqrt(4*pi) * H * W / lambda^2
	// 且 c = 3.0e8。
	const double c0 = 3.0e8;

	const double height = (AntennaHeight > 1.0e-12) ? AntennaHeight : 1.0e-12;
	const double width = (AntennaWidth > 1.0e-12) ? AntennaWidth : 1.0e-12;

	const double lambda = c0 / fcHz;
	const double area = height * width;

	double gain = std::sqrt(4.0 * M_PI) * area / (lambda * lambda);

	if (gain < 0.0 || gain != gain)
	{
		gain = 1.0;
	}

	return gain;
}


double RADAR_Antenna_Rx::calcDistributionWeight(double uNorm) const
{
	const double u = clampValue(uNorm, 0.0, 1.0);

	switch (Pattern)
	{
	case UserDefinedPattern:
		return 1.0;

	case Uniform:
		return 1.0;

	case Cosine:
	{
		const double n = (Factor1 > 0.0) ? Factor1 : 1.0;
		const double w = std::cos(0.5 * M_PI * u);
		return std::pow(clampValue(w, 0.0, 1.0), n);
	}

	case Parabolic:
	{
		const double n = (Factor1 > 0.0) ? Factor1 : 1.0;
		const double w = 1.0 - u * u;
		return std::pow(clampValue(w, 0.0, 1.0), n);
	}

	case Triangle:
	{
		const double n = (Factor1 > 0.0) ? Factor1 : 1.0;
		const double w = 1.0 - u;
		return std::pow(clampValue(w, 0.0, 1.0), n);
	}

	case Circular:
	{
		const double n = (Factor1 > 0.0) ? Factor1 : 1.0;
		const double w = std::sqrt(clampValue(1.0 - u * u, 0.0, 1.0));
		return std::pow(clampValue(w, 0.0, 1.0), n);
	}

	case CosineSquaredPedestal:
	{
		double pedestal = Factor1;
		pedestal = clampValue(pedestal, 0.0, 1.0);

		const double n = (Factor2 > 0.0) ? Factor2 : 1.0;
		const double c = std::cos(0.5 * M_PI * u);
		const double taper = std::pow(clampValue(c * c, 0.0, 1.0), n);

		return pedestal + (1.0 - pedestal) * taper;
	}

	case Taylor:
	{
		const double atten = std::fabs(Sidelobe_Levels);
		const double beta = (atten > 0.0) ? std::sqrt(atten) : 0.0;
		const double arg = beta * std::sqrt(clampValue(1.0 - u * u, 0.0, 1.0));

		double denom = besselI0(beta);
		if (std::fabs(denom) < 1.0e-300)
		{
			denom = 1.0;
		}

		double w = besselI0(arg) / denom;

		if (nBar > 1)
		{
			w = std::pow(clampValue(w, 0.0, 1.0), 1.0 / static_cast<double>(nBar));
		}

		return clampValue(w, 0.0, 1.0);
	}

	default:
		return 1.0;
	}
}