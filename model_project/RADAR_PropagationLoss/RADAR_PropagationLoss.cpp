#include "RADAR_PropagationLoss.h"

#include <algorithm>
#include <cmath>

#ifndef M_PI
#define M_PI 3.141592653589793238462643383279502884
#endif

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(RADAR_PropagationLoss)
{
	SET_MODEL_DESCRIPTION("Propagation loss through rainfall or snowfall");
	SET_MODEL_CATEGORY("Environments");

	// ============================================================
	// 端口定义
	// ============================================================
	{
		SystemVueModelBuilder::DFPort p = ADD_MODEL_INPUT(range);
		p.SetName("range");
		p.SetDescription("Range in meters");
	}

	{
		SystemVueModelBuilder::DFPort p = ADD_MODEL_OUTPUT(attenuate);
		p.SetName("attenuate");
		p.SetDescription("Attenuate value");
	}

	// ============================================================
	// 参数定义
	// ============================================================
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_ENUM_PARAM(OutputUnit, RADAR_PropagationLoss::OutputUnitEnum);
		p.SetName("OutputUnit");
		p.AddEnumeration("dB", RADAR_PropagationLoss::Output_dB);
		p.AddEnumeration("Linear Power Loss", RADAR_PropagationLoss::Linear_Power_Loss);
		p.AddEnumeration("Linear Amplitude Loss", RADAR_PropagationLoss::Linear_Amplitude_Loss);
		p.SetDefaultValue("0");
		p.SetDescription("The propagation loss output unit: dB, Linear Power Loss, Linear Amplitude Loss");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_ENUM_PARAM(PropagationLossType, RADAR_PropagationLoss::PropagationLossTypeEnum);
		p.SetName("PropagationLossType");
		p.AddEnumeration("Rainfall", RADAR_PropagationLoss::Rainfall);
		p.AddEnumeration("Snowfall", RADAR_PropagationLoss::Snowfall);
		p.AddEnumeration("77GHz Rainfall", RADAR_PropagationLoss::Rainfall_77GHz);
		p.SetDefaultValue("0");
		p.SetDescription("The propagation loss type: Rainfall, Snowfall, 77GHz Rainfall");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_ENUM_PARAM(RainLoss77GHzType, RADAR_PropagationLoss::RainLoss77GHzTypeEnum);
		p.SetName("RainLoss77GHzType");
		p.AddEnumeration("Near Range Loss", RADAR_PropagationLoss::Near_Range_Loss);
		p.AddEnumeration("Mid Range Loss", RADAR_PropagationLoss::Mid_Range_Loss);
		p.AddEnumeration("Antenna Water Layer Loss", RADAR_PropagationLoss::Antenna_Water_Layer_Loss);
		p.SetDefaultValue("1");
		p.SetDescription("The 77 GHz rain loss type: Near Range Loss, Mid Range Loss, Antenna Water Layer Loss");
		// 仅 PropagationLossType = 2:77GHz Rainfall 时显示。
		p.SetHideCondition("PropagationLossType ~= 2");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(Frequency);
		p.SetName("Frequency");
		p.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		p.SetDefaultValue("77e9");
		p.SetDescription("Carrier Frequency");
		// 仅 77GHz Rainfall 的 Near Range Loss 和 Antenna Water Layer Loss 显示；
		// Mid Range Loss 不显示 Frequency。
		p.SetHideCondition("PropagationLossType ~= 2 || RainLoss77GHzType == 1");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(RainfallRate);
		p.SetName("RainfallRate");
		p.SetDefaultValue("15");
		p.SetDescription("The rainfall rate, the unit is mm/hour");
		// Rainfall 分支显示；77GHz Rainfall 的 Near/Mid 显示；
		// Snowfall 和 Antenna Water Layer Loss 隐藏。
		p.SetHideCondition("PropagationLossType == 1 || (PropagationLossType == 2 && RainLoss77GHzType == 2)");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(AntTheta);
		p.SetName("AntTheta");
		p.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		p.SetDefaultValue("3");
		p.SetDescription("The antenna elevation 3-dB beamwidth in deg");
		// 仅 77GHz Rainfall / Near Range Loss 显示。
		p.SetHideCondition("PropagationLossType ~= 2 || RainLoss77GHzType ~= 0");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(AntPhi);
		p.SetName("AntPhi");
		p.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		p.SetDefaultValue("4");
		p.SetDescription("The antenna azimuth 3-dB beamwidth in deg");
		// 仅 77GHz Rainfall / Near Range Loss 显示。
		p.SetHideCondition("PropagationLossType ~= 2 || RainLoss77GHzType ~= 0");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(AntHeight);
		p.SetName("AntHeight");
		p.SetUnit(SystemVueModelBuilder::Units::LENGTH);
		p.SetDefaultValue("0.5");
		p.SetDescription("The antenna height above ground in meter");
		// 仅 77GHz Rainfall / Near Range Loss 显示。
		p.SetHideCondition("PropagationLossType ~= 2 || RainLoss77GHzType ~= 0");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(Bandwidth);
		p.SetName("Bandwidth");
		p.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		p.SetDefaultValue("500e6");
		p.SetDescription("The automotive radar bandwidth in Hz");
		// 仅 77GHz Rainfall / Near Range Loss 显示。
		p.SetHideCondition("PropagationLossType ~= 2 || RainLoss77GHzType ~= 0");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(TarRCS);
		p.SetName("TarRCS");
		p.SetDefaultValue("10");
		p.SetDescription("The target RCS in m^2");
		// 仅 77GHz Rainfall / Near Range Loss 显示。
		p.SetHideCondition("PropagationLossType ~= 2 || RainLoss77GHzType ~= 0");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_ENUM_PARAM(TempAntWtLyLoss, RADAR_PropagationLoss::TempAntWtLyLossEnum);
		p.SetName("TempAntWtLyLoss");
		p.AddEnumeration("-10", RADAR_PropagationLoss::Temp_minus10);
		p.AddEnumeration("0", RADAR_PropagationLoss::Temp_0);
		p.AddEnumeration("10", RADAR_PropagationLoss::Temp_10);
		p.AddEnumeration("20", RADAR_PropagationLoss::Temp_20);
		p.AddEnumeration("30", RADAR_PropagationLoss::Temp_30);
		p.AddEnumeration("40", RADAR_PropagationLoss::Temp_40);
		p.AddEnumeration("50", RADAR_PropagationLoss::Temp_50);
		p.SetDefaultValue("3");
		p.SetDescription("Temperature (Celsius) in antenna water layer Loss calculation: -10, 0, 10, 20, 30, 40, 50");
		// 仅 77GHz Rainfall / Antenna Water Layer Loss 显示。
		p.SetHideCondition("PropagationLossType ~= 2 || RainLoss77GHzType ~= 2");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(dw);
		p.SetName("dw");
		p.SetUnit(SystemVueModelBuilder::Units::LENGTH);
		p.SetDefaultValue("0.25e-3");
		p.SetDescription("Antenna water layer thickness, generally between 0~1 mm");
		// 仅 77GHz Rainfall / Antenna Water Layer Loss 显示。
		p.SetHideCondition("PropagationLossType ~= 2 || RainLoss77GHzType ~= 2");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(SnowfallRate);
		p.SetName("SnowfallRate");
		p.SetDefaultValue("1.5");
		p.SetDescription("The snowfall rate in equivalent rainfall water content, the unit is mm/hour. As a general rule, the ratio between snowfall rate and its water equivalent in rain is 10:1");
		// 仅 Snowfall 分支显示。
		p.SetHideCondition("PropagationLossType ~= 1");
	}

	return true;
}
#endif


// ============================================================
// 构造函数
// ============================================================
RADAR_PropagationLoss::RADAR_PropagationLoss()
	: range()
	, attenuate()
	, OutputUnit(Output_dB)
	, PropagationLossType(Rainfall)
	, RainLoss77GHzType(Mid_Range_Loss)
	, Frequency(77e9)
	, RainfallRate(15.0)
	, AntTheta(3.0)
	, AntPhi(4.0)
	, AntHeight(0.5)
	, Bandwidth(500e6)
	, TarRCS(10.0)
	, TempAntWtLyLoss(Temp_20)
	, dw(0.25e-3)
	, SnowfallRate(1.5)
{
}


// ============================================================
// Setup
// ============================================================
bool RADAR_PropagationLoss::Setup()
{
	range.SetRate(1u);
	attenuate.SetRate(1u);

	if (Frequency <= 0.0)
	{
		POST_ERROR("Frequency must be greater than 0.");
		return false;
	}

	// 普通 Rainfall 分支中 RainfallRate=0 会报错。
	if (PropagationLossType == Rainfall)
	{
		if (RainfallRate <= 0.0)
		{
			POST_ERROR("RainfallRate must be greater than 0.");
			return false;
		}
	}

	// Snowfall 分支中 SnowfallRate=0 会报错。
	if (PropagationLossType == Snowfall)
	{
		if (SnowfallRate <= 0.0)
		{
			POST_ERROR("SnowfallRate must be greater than 0.");
			return false;
		}
	}

	// 77GHz Rainfall 的 Near/Mid 分支需要 RainfallRate；
	// Antenna Water Layer Loss 分支不使用 RainfallRate。
	if (PropagationLossType == Rainfall_77GHz &&
		RainLoss77GHzType != Antenna_Water_Layer_Loss)
	{
		if (RainfallRate <= 0.0)
		{
			POST_ERROR("RainfallRate must be greater than 0.");
			return false;
		}
	}

	if (PropagationLossType == Rainfall_77GHz &&
		RainLoss77GHzType == Near_Range_Loss)
	{
		if (Bandwidth <= 0.0)
		{
			POST_ERROR("Bandwidth must be greater than 0.");
			return false;
		}

		if (TarRCS <= 0.0)
		{
			POST_ERROR("TarRCS must be greater than 0.");
			return false;
		}

		if (AntTheta <= 0.0)
		{
			POST_ERROR("AntTheta must be greater than 0.");
			return false;
		}

		if (AntPhi <= 0.0)
		{
			POST_ERROR("AntPhi must be greater than 0.");
			return false;
		}
	}

	if (PropagationLossType == Rainfall_77GHz &&
		RainLoss77GHzType == Antenna_Water_Layer_Loss)
	{
		if (dw < 0.0)
		{
			POST_ERROR("dw must be greater than or equal to 0.");
			return false;
		}
	}

	return true;
}


// ============================================================
// Run
// ============================================================
bool RADAR_PropagationLoss::Run()
{
	const double r = range[0];

	const double lossDb = computeLossDb_(r);
	attenuate[0] = convertOutputUnit_(lossDb);

	return true;
}


// ============================================================
// 根据枚举计算 dB 损耗
// ============================================================
double RADAR_PropagationLoss::computeLossDb_(double rangeMeter) const
{
	const double r = std::max(0.0, rangeMeter);

	switch (PropagationLossType)
	{
	case Rainfall:
		return genericRainLossDb_(r);

	case Snowfall:
		return genericSnowLossDb_(r);

	case Rainfall_77GHz:
		return rain77GHzLossDb_(r);

	default:
		return genericRainLossDb_(r);
	}
}


// ============================================================
// 普通 Rainfall 分支
// ============================================================
double RADAR_PropagationLoss::genericRainLossDb_(double rangeMeter) const
{
	const double rain = RainfallRate;
	const double r = std::max(0.0, rangeMeter);

	if (r <= 0.0)
		return 0.0;

	// 内部函数返回正损耗量，输出阶段再转成负衰减 dB。
	const double coeff = 0.004162; // dB / m / (mm/hour)

	return std::max(0.0, coeff * rain * r);
}


// ============================================================
// Snowfall 分支
// ============================================================
double RADAR_PropagationLoss::genericSnowLossDb_(double rangeMeter) const
{
	const double snow = SnowfallRate;
	const double r = std::max(0.0, rangeMeter);

	if (r <= 0.0)
		return 0.0;

	// 内部函数返回正损耗量，输出阶段再转成负衰减 dB。
	const double coeff = 4.57e-5;
	const double exponent = 2.588;

	return std::max(0.0, coeff * r * std::pow(snow, exponent));
}


// ============================================================
// 77GHz Rainfall 总分支
// ============================================================
double RADAR_PropagationLoss::rain77GHzLossDb_(double rangeMeter) const
{
	switch (RainLoss77GHzType)
	{
	case Near_Range_Loss:
		return rain77GHzNearLossDb_(rangeMeter);

	case Mid_Range_Loss:
		return rain77GHzMidLossDb_(rangeMeter);

	case Antenna_Water_Layer_Loss:
		return antennaWaterLayerLossDb_();

	default:
		return rain77GHzMidLossDb_(rangeMeter);
	}
}


// ============================================================
// 77GHz Near Range Loss
// ============================================================
double RADAR_PropagationLoss::rain77GHzNearLossDb_(double rangeMeter) const
{
	const double rain = std::max(0.0, RainfallRate);
	if (rangeMeter <= 0.0 || rain <= 0.0)
		return 0.0;

	const double freqGHz = safePositive_(Frequency / 1.0e9, 77.0);

	// 基础雨衰。77GHz 近距离仍有一程路径衰减。
	const double gammaPath = ituRainSpecificAttenDbPerKm_(freqGHz, rain);
	const double pathLossDb = gammaPath * (rangeMeter / 1000.0);

	// 估算距离分辨率，单位 m。
	const double c0 = 299792458.0;
	const double bw = safePositive_(Bandwidth, 500e6);
	const double dr = c0 / (2.0 * bw);

	// 天线波束宽度，单位 rad。
	const double theta = std::max(deg2rad_(std::abs(AntTheta)), 1.0e-6);
	const double phi = std::max(deg2rad_(std::abs(AntPhi)), 1.0e-6);

	// 近距离雨杂波体积近似。
	const double volume = rangeMeter * rangeMeter * theta * phi * dr;

	// 经验雨后向散射体系数，单位为近似 1/m。
	const double eta = 1.0e-8 *
		std::pow(rain / 15.0, 1.60) *
		std::pow(clamp_(freqGHz / 77.0, 0.2, 5.0), 4.0);

	double rainClutterRcs = eta * volume;

	// AntHeight 用于近地车载雷达几何修正。
	const double h = std::max(0.0, AntHeight);
	const double heightFactor = clamp_(1.0 + 0.5 / (h + 0.5), 1.0, 2.0);
	rainClutterRcs *= heightFactor;

	const double targetRcs = std::max(TarRCS, 1.0e-12);

	// 附加 SNR loss。雨杂波相对目标越大，损失越大。
	const double clutterLossDb = 10.0 * log10Safe_(1.0 + rainClutterRcs / targetRcs);

	return std::max(0.0, pathLossDb + clutterLossDb);
}


// ============================================================
// 77GHz Mid Range Loss
// ============================================================
double RADAR_PropagationLoss::rain77GHzMidLossDb_(double rangeMeter) const
{
	const double rain = std::max(0.0, RainfallRate);
	if (rangeMeter <= 0.0 || rain <= 0.0)
		return 0.0;

	const double gamma = rain77MidSpecificAttenDbPerKm_(rain);
	return std::max(0.0, gamma * (rangeMeter / 1000.0));
}


// ============================================================
// 77GHz Antenna Water Layer Loss
// ============================================================
double RADAR_PropagationLoss::antennaWaterLayerLossDb_() const
{
	const double dwMeter = std::max(0.0, dw);

	// V2 修正：dw=0 时内置严格输出 0。
	if (dwMeter <= 1.0e-12)
		return 0.0;

	const double freqGHz = safePositive_(Frequency / 1.0e9, 77.0);
	const double dwMm = dwMeter * 1000.0;

	// 77GHz 水膜损耗工程量级：约 30 dB/mm。
	double dbPerMm = 30.0 * std::pow(clamp_(freqGHz / 77.0, 0.2, 5.0), 0.80);

	// 温度修正：20°C 作为基准。
	const double t = tempCelsius_();
	const double tempFactor = clamp_(1.0 + 0.004 * (20.0 - t), 0.70, 1.30);

	return std::max(0.0, dbPerMm * dwMm * tempFactor);
}


// ============================================================
// 输出单位转换
// ============================================================
double RADAR_PropagationLoss::convertOutputUnit_(double lossDb) const
{
	double L = std::max(0.0, lossDb);

	if (L < 1.0e-12)
		L = 0.0;

	// 内置约定：dB 输出为负衰减值。
	const double attenDb = -L;

	switch (OutputUnit)
	{
	case Output_dB:
		return attenDb;

	case Linear_Power_Loss:
		return std::pow(10.0, attenDb / 10.0);

	case Linear_Amplitude_Loss:
		return std::pow(10.0, attenDb / 20.0);

	default:
		return attenDb;
	}
}


// ============================================================
// ITU-R P.838-3 风格水平极化雨衰比衰减
// ============================================================
double RADAR_PropagationLoss::ituRainSpecificAttenDbPerKm_(double freqGHz, double rainRate) const
{
	const double f = clamp_(freqGHz, 1.0, 1000.0);
	const double R = std::max(0.0, rainRate);

	if (R <= 0.0)
		return 0.0;

	const double x = std::log10(f);

	// kH coefficients
	const double ak[4] = { -5.33980, -0.35351, -0.23789, -0.94158 };
	const double bk[4] = { -0.10008,  1.26970,  0.86036,  0.64552 };
	const double ck[4] = { 1.13098,  0.45400,  0.15354,  0.16817 };
	const double mk = -0.18961;
	const double ck0 = 0.71147;

	double logk = mk * x + ck0;
	for (int j = 0; j < 4; ++j)
	{
		const double u = (x - bk[j]) / ck[j];
		logk += ak[j] * std::exp(-u * u);
	}

	const double k = std::pow(10.0, logk);

	// alphaH coefficients
	const double aa[5] = { -0.14318, 0.29591, 0.32177, -5.37610, 16.17210 };
	const double ba[5] = { 1.82442, 0.77564, 0.63773, -0.96230, -3.29980 };
	const double ca[5] = { -0.55187, 0.19822, 0.13164,  1.47828,  3.43990 };
	const double ma = 0.67849;
	const double ca0 = -1.95537;

	double alpha = ma * x + ca0;
	for (int j = 0; j < 5; ++j)
	{
		const double u = (x - ba[j]) / ca[j];
		alpha += aa[j] * std::exp(-u * u);
	}

	return std::max(0.0, k * std::pow(R, alpha));
}


// ============================================================
// 77GHz 中距离经验比衰减，单位 dB/km
// ============================================================
double RADAR_PropagationLoss::rain77MidSpecificAttenDbPerKm_(double rainRate) const
{
	const double R = std::max(0.0, rainRate);

	// 经验量级表。用于 77GHz Mid Range Loss 初版对齐。
	static const double xs[] = { 0.0, 5.0, 20.0, 50.0, 100.0 };
	static const double ys[] = { 0.0, 4.0, 10.0, 20.0, 35.0 };

	return interp1_(xs, ys, 5, R);
}


// ============================================================
// 温度枚举转摄氏度
// ============================================================
double RADAR_PropagationLoss::tempCelsius_() const
{
	switch (TempAntWtLyLoss)
	{
	case Temp_minus10: return -10.0;
	case Temp_0:       return 0.0;
	case Temp_10:      return 10.0;
	case Temp_20:      return 20.0;
	case Temp_30:      return 30.0;
	case Temp_40:      return 40.0;
	case Temp_50:      return 50.0;
	default:           return 20.0;
	}
}


double RADAR_PropagationLoss::deg2rad_(double x)
{
	return x * M_PI / 180.0;
}


double RADAR_PropagationLoss::clamp_(double x, double lo, double hi)
{
	if (x != x) return lo;
	if (x < lo) return lo;
	if (x > hi) return hi;
	return x;
}


double RADAR_PropagationLoss::safePositive_(double x, double fallback)
{
	if (!(x > 0.0))
		return fallback;

	if (!std::isfinite(x))
		return fallback;

	return x;
}


double RADAR_PropagationLoss::log10Safe_(double x)
{
	if (!(x > 0.0))
		x = 1.0e-300;

	return std::log10(x);
}


double RADAR_PropagationLoss::interp1_(const double* xs, const double* ys, int n, double x)
{
	if (n <= 0 || xs == 0 || ys == 0)
		return 0.0;

	if (n == 1)
		return ys[0];

	if (x <= xs[0])
	{
		const double dx = xs[1] - xs[0];
		if (std::abs(dx) < 1.0e-30) return ys[0];
		const double t = (x - xs[0]) / dx;
		return ys[0] + t * (ys[1] - ys[0]);
	}

	for (int i = 0; i < n - 1; ++i)
	{
		if (x >= xs[i] && x <= xs[i + 1])
		{
			const double dx = xs[i + 1] - xs[i];
			if (std::abs(dx) < 1.0e-30) return ys[i];
			const double t = (x - xs[i]) / dx;
			return ys[i] + t * (ys[i + 1] - ys[i]);
		}
	}

	const double dx = xs[n - 1] - xs[n - 2];
	if (std::abs(dx) < 1.0e-30) return ys[n - 1];

	const double t = (x - xs[n - 2]) / dx;
	return ys[n - 2] + t * (ys[n - 1] - ys[n - 2]);
}
