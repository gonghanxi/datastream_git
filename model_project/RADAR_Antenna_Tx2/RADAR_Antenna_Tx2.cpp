#include "RADAR_Antenna_Tx2.h"

#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(RADAR_Antenna_Tx2)
{
	SET_MODEL_DESCRIPTION("Transmitting Antenna");
	SET_MODEL_SYMBOL("SYM_RADAR_Antenna_Tx2@RADAR Symbols");
	SET_MODEL_CATEGORY("Signal Processing");

	// =========================
	// 端口
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
		p.SetOptional(false);
	}
	{
		SystemVueModelBuilder::DFPort p = ADD_MODEL_OUTPUT(output);
		p.SetDescription("The output signal of antenna in the Tx chain");
		p.SetOptional(false);
	}

	// =========================
	// 参数
	// =========================
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_ENUM_PARAMETER(RadarWorkMode, SelectedRadarWorkMode);
		p.AddEnumeration("Tracking", Tracking);
		p.AddEnumeration("Search", Search);
		p.SetDescription("Radar Work Mode : Tracking, Search");
		p.SetDefaultValue("0");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_ENUM_PARAMETER(Pattern, SelectedPattern);
		p.AddEnumeration("UserDefinedPattern", UserDefinedPattern);
		p.AddEnumeration("Uniform", Uniform);
		p.AddEnumeration("Cosine", Cosine);
		p.AddEnumeration("Parabolic", Parabolic);
		p.AddEnumeration("Triangle", Triangle);
		p.AddEnumeration("Circular", Circular);
		p.AddEnumeration("CosineSquaredPedestal", CosineSquaredPedestal);
		p.AddEnumeration("Taylor", Taylor);
		p.SetDescription("User-defined antenna pattern or antenna pattern which is decided by illumination distribution function: UserDefinedPattern, Uniform, Cosine, Parabolic, Triangle, Circular, CosineSquaredPedestal, Taylor");
		p.SetDefaultValue("1");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAMETER(Factor1);
		p.SetDescription("The factor which is defined in the distribution function");
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("0");

		// Pattern = Cosine / Parabolic / CosineSquaredPedestal 时显示 Factor1
		p.SetHideCondition("Pattern ~= 2 && Pattern ~= 3 && Pattern ~= 6");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAMETER(Factor2);
		p.SetDescription("The factor which is defined in the distribution function");
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("0");

		// Pattern = CosineSquaredPedestal 时显示 Factor2
		p.SetHideCondition("Pattern ~= 6");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_ARRAY_PARAMETER(AntennaPatternArray, AntennaPatternArray_Size);
		p.SetDescription("Antenna Pattern: elevation * azimuth with dB");
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("[ones(180 * 360, 1)]");

		// Pattern = UserDefinedPattern 时显示
		p.SetHideCondition("Pattern ~= 0");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAMETER(Sidelobe_Levels);
		p.SetDescription("Sidelobe_levels in dB");
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("-20");

		// Pattern = Taylor 时显示
		p.SetHideCondition("Pattern ~= 7");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAMETER(nBar);
		p.SetDescription("This parameter is used to generate the Taylor distribution");
		p.SetDefaultValue("2");

		// Pattern = Taylor 时显示
		p.SetHideCondition("Pattern ~= 7");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAMETER(AntennaHeight);
		p.SetDescription("The vertical length of antenna");
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("5");

		// UserDefinedPattern 使用 AntennaPatternArray，不显示尺寸参数
		p.SetHideCondition("Pattern == 0");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAMETER(AntennaWidth);
		p.SetDescription("The horizontal length of antenna");
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("5");

		// UserDefinedPattern 使用 AntennaPatternArray，不显示尺寸参数
		p.SetHideCondition("Pattern == 0");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_ENUM_PARAMETER(AntennaScanPattern, SelectedAntennaScanPattern);
		p.AddEnumeration("Circular", CircularScan);
		p.AddEnumeration("Bidirectional Sector", BidirectionalSector);
		p.AddEnumeration("Unidirectional Sector", UnidirectionalSector);
		p.AddEnumeration("Bidirectional Raster", BidirectionalRaster);
		p.AddEnumeration("Unidirectional Raster", UnidirectionalRaster);
		p.SetDescription("Antenna Scan Pattern: Circular, Bidirectional Sector, Unidirectional Sector, Bidirectional Raster, Unidirectional Raster");
		p.SetDefaultValue("0");

		// 仅 Search 模式显示扫描方式
		p.SetHideCondition("RadarWorkMode ~= 1");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAMETER(ScanRate);
		p.SetDescription("Scan Rate sets the antenna scan rate, the unit is rpm(round per minute).");
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("15");

		// 仅 Search 模式显示
		p.SetHideCondition("RadarWorkMode ~= 1");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAMETER(ElevationAngle);
		p.SetDescription("Elevation Angle Value in degree");
		p.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		p.SetDefaultValue("0");

		// 仅 Search 模式显示
		p.SetHideCondition("RadarWorkMode ~= 1");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAMETER(SectorScanStartAngle);
		p.SetDescription("The start angle of scan sector");
		p.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		p.SetDefaultValue("0");

		// Search 且 Sector/Raster 模式显示；Circular 不显示
		p.SetHideCondition("RadarWorkMode ~= 1 || AntennaScanPattern == 0");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAMETER(SectorScanEndAngle);
		p.SetDescription("The end angle of scan sector");
		p.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		p.SetDefaultValue("0");

		// Search 且 Sector/Raster 模式显示；Circular 不显示
		p.SetHideCondition("RadarWorkMode ~= 1 || AntennaScanPattern == 0");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAMETER(FlybackTime);
		p.SetDescription("The flyback time from the single row scan end position to start position");
		p.SetUnit(SystemVueModelBuilder::Units::TIME);
		p.SetDefaultValue("0");

		// Unidirectional Sector / Unidirectional Raster 显示
		p.SetHideCondition("RadarWorkMode ~= 1 || (AntennaScanPattern ~= 2 && AntennaScanPattern ~= 4)");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAMETER(RetraceTime);
		p.SetDescription("The retrace time from the total scan end position to start position");
		p.SetUnit(SystemVueModelBuilder::Units::TIME);
		p.SetDefaultValue("0");

		// Raster 模式显示
		p.SetHideCondition("RadarWorkMode ~= 1 || (AntennaScanPattern ~= 3 && AntennaScanPattern ~= 4)");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAMETER(NumberOfRasterBars);
		p.SetDescription("The number of raster bars when using Bidirectional Raster or Unidirectional Raster scan (Starts from Bar No.0)");
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
		SystemVueModelBuilder::DFParam p = ADD_MODEL_ARRAY_PARAMETER(TargetAzimuthAngle, TargetAzimuthAngle_Size);
		p.SetDescription("The azimuth angle of target related to the radar reference coordinate");
		p.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		p.SetDefaultValue("[0]");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_ARRAY_PARAMETER(TargetElevationAngle, TargetElevationAngle_Size);
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

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAMETER(AntennaEfficiency);
		p.SetDescription("The efficiency is the antenna total efficiency, including:impedance mismatch loss, aperture efficiency and so on");
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("1");

		// 帮助文档中 AntennaEfficiency 为 Tx2 新增参数，始终显示。
	}

	return true;
}
#endif


RADAR_Antenna_Tx2::RADAR_Antenna_Tx2()
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
	, RetraceTime(0.0)
	, NumberOfRasterBars(0)
	, RasterBarWidth(5.0)
	, TargetAzimuthAngle(0)
	, TargetAzimuthAngle_Size(0)
	, TargetElevationAngle(0)
	, TargetElevationAngle_Size(0)
	, BeamAzimuthAngle(0.0)
	, BeamElevationAngle(0.0)
	, AntennaEfficiency(1.0)
{
}


bool RADAR_Antenna_Tx2::Setup()
{
	// input 为普通 envelope 端口，可以直接 SetRate。
	input.SetRate(1);

	// BeamAzimuth / BeamElevation 为普通 real 端口，可以直接 SetRate。
	BeamAzimuth.SetRate(1);
	BeamElevation.SetRate(1);

	// TargetAzimuth / TargetElevation 是 bus 端口，不能对 bus 本身 SetRate。
	// 只能对子通道 SetRate。
	for (size_t i = 0; i < TargetAzimuth.GetSize(); ++i)
	{
		TargetAzimuth[i].SetRate(1);
	}

	for (size_t i = 0; i < TargetElevation.GetSize(); ++i)
	{
		TargetElevation[i].SetRate(1);
	}

	// output 是 multiple envelope bus，不能对 bus 本身 SetRate。
	// 只能对子通道 SetRate。
	for (size_t i = 0; i < output.GetSize(); ++i)
	{
		output[i].SetRate(1);
	}

	return true;
}


ERESULT RADAR_Antenna_Tx2::PropagateCharacterizationFrequency()
{
	// 注意：CharacterizationFrequency 属于 EnvelopeCircularBuffer，
	// 不是 EnvelopeSignal 样本本身。
	const double fc = input.GetCharacterizationFrequency();

	for (size_t i = 0; i < output.GetSize(); ++i)
	{
		output[i].SetCharacterizationFrequency(fc);
	}

	return true;
}


bool RADAR_Antenna_Tx2::Run()
{
	const int targetCount = determineTargetCount();

	if (targetCount <= 0)
	{
		return true;
	}

	const size_t outBusSize = output.GetSize();
	if (outBusSize == 0)
	{
		return true;
	}

	// 由输入 envelope 端口驱动时间轴。
	// 注意不能写 input[0].GetTime()，EnvelopeSignal 没有 GetTime()。
	double timeNow = 0.0;
	timeNow = input.GetTime(0, GetCount());

	double beamAzRad = 0.0;
	double beamElRad = 0.0;
	getBeamAngle(timeNow, beamAzRad, beamElRad);

	double fcHz = input.GetCharacterizationFrequency();

	// 如果上游没有设置 Fc，则给一个保守默认值，避免 lambda 无效。
	// 实际验证时建议使用 CxToEnv 明确设置 Fc。
	if (fcHz <= 0.0)
	{
		fcHz = 1.0e9;
	}

	const int runCount = std::min(targetCount, static_cast<int>(outBusSize));

	for (int ch = 0; ch < runCount; ++ch)
	{
		double targetAzRad = 0.0;
		double targetElRad = 0.0;

		// Target 角度端口连接时，端口单位为 rad；
		// 未连接时，使用参数，参数单位为 deg。
		if (hasTargetAzimuthPort(ch))
		{
			SystemVueModelBuilder::DoubleCircularBuffer& port = TargetAzimuth[static_cast<size_t>(ch)];
			targetAzRad = port[0];
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
			SystemVueModelBuilder::DoubleCircularBuffer& port = TargetElevation[static_cast<size_t>(ch)];
			targetElRad = port[0];
		}
		else
		{
			targetElRad = deg2rad(getArrayValue(TargetElevationAngle,
				TargetElevationAngle_Size,
				ch,
				0.0));
		}

		const double ampGain = calcAntennaAmplitudeGain(targetAzRad,
			targetElRad,
			beamAzRad,
			beamElRad,
			fcHz);

		output[static_cast<size_t>(ch)][0] = input[0] * ampGain;
	}

	// 多余输出通道清零。
	for (size_t ch = static_cast<size_t>(runCount); ch < outBusSize; ++ch)
	{
		output[ch][0] = input[0] * 0.0;
	}

	return true;
}


// ============================================================================
// 端口连接判断
// 注意：这些函数不要加 const，否则 VS2017 会报 IsConnected/operator[] 的 const 限定错误。
// ============================================================================

int RADAR_Antenna_Tx2::determineTargetCount()
{
	bool hasTargetPort = false;

	for (size_t i = 0; i < TargetAzimuth.GetSize(); ++i)
	{
		SystemVueModelBuilder::DoubleCircularBuffer& port = TargetAzimuth[i];
		if (port.IsConnected())
		{
			hasTargetPort = true;
			break;
		}
	}

	if (!hasTargetPort)
	{
		for (size_t i = 0; i < TargetElevation.GetSize(); ++i)
		{
			SystemVueModelBuilder::DoubleCircularBuffer& port = TargetElevation[i];
			if (port.IsConnected())
			{
				hasTargetPort = true;
				break;
			}
		}
	}

	if (hasTargetPort)
	{
		return static_cast<int>(std::max(TargetAzimuth.GetSize(),
			TargetElevation.GetSize()));
	}

	int n1 = TargetAzimuthAngle_Size;
	int n2 = TargetElevationAngle_Size;

	int n = std::max(n1, n2);
	if (n <= 0)
	{
		n = 1;
	}

	return n;
}


bool RADAR_Antenna_Tx2::hasBeamAzimuthPort()
{
	return BeamAzimuth.IsConnected();
}


bool RADAR_Antenna_Tx2::hasBeamElevationPort()
{
	return BeamElevation.IsConnected();
}


bool RADAR_Antenna_Tx2::hasTargetAzimuthPort(int ch)
{
	if (ch < 0)
	{
		return false;
	}

	if (static_cast<size_t>(ch) >= TargetAzimuth.GetSize())
	{
		return false;
	}

	SystemVueModelBuilder::DoubleCircularBuffer& port =
		TargetAzimuth[static_cast<size_t>(ch)];

	return port.IsConnected();
}


bool RADAR_Antenna_Tx2::hasTargetElevationPort(int ch)
{
	if (ch < 0)
	{
		return false;
	}

	if (static_cast<size_t>(ch) >= TargetElevation.GetSize())
	{
		return false;
	}

	SystemVueModelBuilder::DoubleCircularBuffer& port =
		TargetElevation[static_cast<size_t>(ch)];

	return port.IsConnected();
}


double RADAR_Antenna_Tx2::getArrayValue(const double* data,
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
// Beam 角度
// ============================================================================

void RADAR_Antenna_Tx2::getBeamAngle(double timeNow,
	double& beamAzRad,
	double& beamElRad)
{
	// Beam 端口连接时，端口单位为 rad。
	// Beam 参数单位为 deg。
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


double RADAR_Antenna_Tx2::getCircularScanAzimuth(double timeNow) const
{
	// ScanRate 单位：rpm
	// 1 rpm = 360 deg / 60 s = 6 deg/s
	const double rateDegPerSec = ScanRate * 6.0;

	if (rateDegPerSec == 0.0)
	{
		return 0.0;
	}

	return wrap360(rateDegPerSec * timeNow);
}


double RADAR_Antenna_Tx2::getSectorScanAzimuth(double timeNow,
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
		else
		{
			return endDeg - dir * rateDegPerSec * (t - forwardTime);
		}
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


void RADAR_Antenna_Tx2::getRasterScanAngle(double timeNow,
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
	const double rt = std::max(0.0, RetraceTime);

	double rowTime = 0.0;
	if (bidirectional)
	{
		rowTime = scanTime;
	}
	else
	{
		rowTime = scanTime + fb;
	}

	const double activeTime = rowTime * barCount;
	const double period = activeTime + rt;

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

	if (t >= activeTime)
	{
		// 总回扫期间，保守处理为回到起始扫描位置。
		azDeg = startDeg;
		elDeg = ElevationAngle;
		return;
	}

	int row = static_cast<int>(t / rowTime);
	if (row >= barCount)
	{
		row = barCount - 1;
	}

	double rowLocal = t - row * rowTime;

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
// 天线增益
// ============================================================================

double RADAR_Antenna_Tx2::calcAntennaAmplitudeGain(double targetAzRad,
	double targetElRad,
	double beamAzRad,
	double beamElRad,
	double fcHz) const
{
	// 目标在天线背面时，增益置 0。
	// 帮助文档说明：When the target is behind the antenna plane, the gain is set to 0.
	const double sep = angularSeparation(targetAzRad,
		targetElRad,
		beamAzRad,
		beamElRad);

	if (sep > 0.5 * M_PI)
	{
		return 0.0;
	}

	// Tx2 新增的天线总效率。
	// 黑盒测试结果：
	// eta = 0.25 -> 输出约为 Tx 输出 * sqrt(0.25)
	// eta = 0.5  -> 输出约为 Tx 输出 * sqrt(0.5)
	// eta = 50   -> 输出约为 Tx 输出 * sqrt(50)
	// eta = 100  -> 输出约为 Tx 输出 * sqrt(100)
	// 因此这里不按百分比除以 100，也不限制 eta 上限。
	double eta = AntennaEfficiency;
	if (eta < 0.0)
	{
		eta = 0.0;
	}

	const double efficiencyAmpGain = std::sqrt(eta);

	// UserDefinedPattern 中，AntennaPatternArray 表示 dB 方向图。
	// Tx2 的 AntennaEfficiency 是总效率参数，因此对用户自定义方向图也作为最终幅度效率因子叠加。
	if (Pattern == UserDefinedPattern)
	{
		return efficiencyAmpGain *
			calcUserPatternGain(targetAzRad,
				targetElRad,
				beamAzRad,
				beamElRad);
	}

	// SystemVue 雷达内置更接近使用工程光速 3e8。
	// 对 H=5, W=5, fc=10GHz：
	// lambda = 0.03
	// sqrt(4*pi*H*W)/lambda ≈ 590.818
	const double c = 3.0e8;

	if (fcHz <= 0.0)
	{
		return 0.0;
	}

	const double lambda = c / fcHz;

	if (lambda <= 0.0)
	{
		return 0.0;
	}

	if (AntennaHeight <= 0.0 || AntennaWidth <= 0.0)
	{
		return 0.0;
	}

	const double apertureArea = AntennaHeight * AntennaWidth;

	// Tx2 的核心差异：
	// RADAR_Antenna_Tx:
	//     G_amp = sqrt(4*pi*A) / lambda
	//
	// RADAR_Antenna_Tx2:
	//     G_amp = sqrt(AntennaEfficiency) * sqrt(4*pi*A) / lambda
	//           = sqrt(AntennaEfficiency * 4*pi*A) / lambda
	//
	// 注意 AntennaEfficiency 是直接数值，不是百分比。
	const double apertureAmpGain =
		efficiencyAmpGain * std::sqrt(4.0 * M_PI * apertureArea) / lambda;

	const double dAz = normalizeRad(targetAzRad - beamAzRad);
	const double dEl = normalizeRad(targetElRad - beamElRad);

	const double patternFactor = calcAnalyticPatternFactor(dAz, dEl, lambda);

	return apertureAmpGain * patternFactor;
}


double RADAR_Antenna_Tx2::calcUserPatternGain(double targetAzRad,
	double targetElRad,
	double beamAzRad,
	double beamElRad) const
{
	if (AntennaPatternArray == 0 || AntennaPatternArray_Size <= 0)
	{
		return 1.0;
	}

	double dAzDeg = rad2deg(normalizeRad(targetAzRad - beamAzRad));
	double dElDeg = rad2deg(normalizeRad(targetElRad - beamElRad));

	// 帮助文档默认 AntennaPatternArray 为 elevation * azimuth：
	// elevation: -90 ~ +90   -> row 0 ~ 179
	// azimuth  : -180 ~ +180 -> col 0 ~ 359
	int elIndex = static_cast<int>(std::floor(dElDeg + 90.0));
	int azIndex = static_cast<int>(std::floor(dAzDeg + 180.0));

	if (elIndex < 0)
	{
		elIndex = 0;
	}

	if (elIndex > 179)
	{
		elIndex = 179;
	}

	while (azIndex < 0)
	{
		azIndex += 360;
	}

	while (azIndex >= 360)
	{
		azIndex -= 360;
	}

	const int index = elIndex * 360 + azIndex;

	double gainDb = 0.0;

	if (index >= 0 && index < AntennaPatternArray_Size)
	{
		gainDb = AntennaPatternArray[index];
	}
	else
	{
		gainDb = AntennaPatternArray[AntennaPatternArray_Size - 1];
	}

	// AntennaPatternArray 中存的是 dB，输出需要转换为包络幅度增益。
	return std::pow(10.0, gainDb / 20.0);
}


double RADAR_Antenna_Tx2::calcAnalyticPatternFactor(double dAzRad,
	double dElRad,
	double lambda) const
{
	if (lambda <= 0.0)
	{
		return 0.0;
	}

	const double u = std::sin(dAzRad);
	const double v = std::sin(dElRad);

	const double xAz = M_PI * AntennaWidth / lambda * u;
	const double xEl = M_PI * AntennaHeight / lambda * v;

	double base = std::fabs(sinc(xAz) * sinc(xEl));

	if (base < 0.0)
	{
		base = 0.0;
	}

	switch (Pattern)
	{
	case Uniform:
		return base;

	case Cosine:
	{
		// Factor1 可理解为 cosine 分布阶数，默认 0 时退化为较温和形式。
		const double n = std::max(0.0, Factor1);
		const double w = std::pow(std::max(0.0, std::cos(0.5 * dAzRad)), n + 1.0) *
			std::pow(std::max(0.0, std::cos(0.5 * dElRad)), n + 1.0);
		return base * w;
	}

	case Parabolic:
	{
		const double delta = Factor1;
		const double r2 = u * u + v * v;
		double w = 1.0 - delta * r2;
		if (w < 0.0)
		{
			w = 0.0;
		}
		return base * w;
	}

	case Triangle:
	{
		const double waz = std::max(0.0, 1.0 - std::fabs(dAzRad) / (0.5 * M_PI));
		const double wel = std::max(0.0, 1.0 - std::fabs(dElRad) / (0.5 * M_PI));
		return base * waz * wel;
	}

	case Circular:
	{
		const double rho = std::sqrt(u * u + v * v);
		const double x = M_PI * std::max(AntennaWidth, AntennaHeight) / lambda * rho;

		if (std::fabs(x) < 1.0e-12)
		{
			return 1.0;
		}

		// 近似圆孔径方向图幅度：2*J1(x)/x。
		// 这里用简化近似，保证不引入额外特殊函数依赖。
		const double approxJ1 = std::sin(x) / (x * x) - std::cos(x) / x;
		return std::fabs(2.0 * approxJ1 / x);
	}

	case CosineSquaredPedestal:
	{
		const double pedestal1 = Factor1;
		const double pedestal2 = Factor2;

		double caz = std::cos(0.5 * dAzRad);
		double cel = std::cos(0.5 * dElRad);

		if (caz < 0.0)
		{
			caz = 0.0;
		}
		if (cel < 0.0)
		{
			cel = 0.0;
		}

		const double waz = pedestal1 + pedestal2 * caz * caz;
		const double wel = pedestal1 + pedestal2 * cel * cel;

		return base * std::max(0.0, waz) * std::max(0.0, wel);
	}

	case Taylor:
	{
		// 简化 Taylor：按旁瓣电平构造一个平滑加权因子。
		const double sidelobeLinear = std::pow(10.0, Sidelobe_Levels / 20.0);
		const double order = std::max(1, nBar);

		const double rho = std::sqrt(u * u + v * v);
		double w = sidelobeLinear +
			(1.0 - sidelobeLinear) * std::pow(std::max(0.0, 1.0 - rho), order);

		if (w < 0.0)
		{
			w = 0.0;
		}

		return base * w;
	}

	default:
		return base;
	}
}


// ============================================================================
// 工具函数
// ============================================================================

double RADAR_Antenna_Tx2::angularSeparation(double az1,
	double el1,
	double az2,
	double el2) const
{
	const double x1 = std::cos(el1) * std::cos(az1);
	const double y1 = std::cos(el1) * std::sin(az1);
	const double z1 = std::sin(el1);

	const double x2 = std::cos(el2) * std::cos(az2);
	const double y2 = std::cos(el2) * std::sin(az2);
	const double z2 = std::sin(el2);

	double dot = x1 * x2 + y1 * y2 + z1 * z2;

	if (dot > 1.0)
	{
		dot = 1.0;
	}
	if (dot < -1.0)
	{
		dot = -1.0;
	}

	return std::acos(dot);
}


double RADAR_Antenna_Tx2::deg2rad(double x)
{
	return x * M_PI / 180.0;
}


double RADAR_Antenna_Tx2::rad2deg(double x)
{
	return x * 180.0 / M_PI;
}


double RADAR_Antenna_Tx2::normalizeRad(double x)
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


double RADAR_Antenna_Tx2::wrap360(double x)
{
	double y = std::fmod(x, 360.0);

	if (y < 0.0)
	{
		y += 360.0;
	}

	return y;
}


double RADAR_Antenna_Tx2::sinc(double x)
{
	if (std::fabs(x) < 1.0e-12)
	{
		return 1.0;
	}

	return std::sin(x) / x;
}


double RADAR_Antenna_Tx2::besselI0(double x)
{
	// 简单级数近似，当前版本保留备用。
	double sum = 1.0;
	double term = 1.0;

	const double xx = x * x / 4.0;

	for (int k = 1; k <= 20; ++k)
	{
		term *= xx / static_cast<double>(k * k);
		sum += term;
	}

	return sum;
}