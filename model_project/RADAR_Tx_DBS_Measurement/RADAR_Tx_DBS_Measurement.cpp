#include "RADAR_Tx_DBS_Measurement.h"

#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.141592653589793238462643383279502884
#endif

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(RADAR_Tx_DBS_Measurement)
{
	SET_MODEL_DESCRIPTION("RF transmitter antenna pattern measurement");
	SET_MODEL_CATEGORY("Environments");

	// ============================================================
	// 端口
	// ============================================================
	{
		auto p = ADD_MODEL_INPUT(input);
		p.SetName("input");
		p.SetDescription("The input signal");
	}

	{
		auto p = ADD_MODEL_OUTPUT(AntPattern);
		p.SetName("AntPattern");
		p.SetDescription("The pattern of phase shift array antenna");
	}

	// ============================================================
	// 参数
	// ============================================================
	{
		auto p = ADD_MODEL_PARAM(PRI);
		p.SetName("PRI");
		p.SetDefaultValue("1e-4");
		p.SetUnit(SystemVueModelBuilder::Units::TIME);
		p.SetDescription("Pulse Repeat Interval");
	}

	{
		auto p = ADD_MODEL_PARAM(SamplingRate);
		p.SetName("SamplingRate");
		p.SetDefaultValue("10e6");
		p.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		p.SetDescription("Waveform Sampling Rate");
	}

	{
		auto p = ADD_MODEL_PARAM(NumOfAntx);
		p.SetName("NumOfAntx");
		p.SetDefaultValue("4");
		p.SetDescription("Number of Antenna in X axis");
	}

	{
		auto p = ADD_MODEL_PARAM(NumOfAnty);
		p.SetName("NumOfAnty");
		p.SetDefaultValue("4");
		p.SetDescription("Number of Antenna in Y axis");
	}

	{
		auto p = ADD_MODEL_PARAM(Dx);
		p.SetName("Dx");
		p.SetDefaultValue("0.5");
		p.SetDescription("Antenna Spacing in wavelengths of X axis");
	}

	{
		auto p = ADD_MODEL_PARAM(Dy);
		p.SetName("Dy");
		p.SetDefaultValue("0.5");
		p.SetDescription("Antenna Spacing in wavelengths of Y axis");
	}

	{
		auto p = ADD_MODEL_ENUM_PARAM(ParamToSweep, RADAR_Tx_DBS_Measurement::ParamToSweepEnum);
		p.SetName("ParamToSweep");
		p.AddEnumeration("Phi", RADAR_Tx_DBS_Measurement::Sweep_Phi);
		p.AddEnumeration("Theta", RADAR_Tx_DBS_Measurement::Sweep_Theta);
		p.SetDefaultValue("Phi");
		p.SetDescription("The parameter to sweep: Phi, Theta");
	}

	{
		auto p = ADD_MODEL_PARAM(Theta_Phi);
		p.SetName("Theta_Phi");
		p.SetDefaultValue("0");
		p.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		p.SetDescription("Array direction angle theta or phi in radians");
	}

	{
		auto p = ADD_MODEL_ENUM_PARAM(TypeOfSweep, RADAR_Tx_DBS_Measurement::TypeOfSweepEnum);
		p.SetName("TypeOfSweep");
		p.AddEnumeration("Linear:Number of Points", RADAR_Tx_DBS_Measurement::Linear_Number_of_Points);
		p.AddEnumeration("Linear:Step Size", RADAR_Tx_DBS_Measurement::Linear_Step_Size);
		p.SetDefaultValue("Linear:Number of Points");
		p.SetDescription("The type of sweep: Linear:Number of Points, Linear:Step Size");
	}

	{
		auto p = ADD_MODEL_PARAM(SweepStart);
		p.SetName("SweepStart");
		p.SetDefaultValue("0");
		p.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		p.SetDescription("The start point for parameter sweep range");
	}

	{
		auto p = ADD_MODEL_PARAM(SweepStop);
		p.SetName("SweepStop");
		p.SetDefaultValue("360");
		p.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		p.SetDescription("The stop point for parameter sweep range");
	}

	{
		auto p = ADD_MODEL_PARAM(SweepNumOfPoints);
		p.SetName("SweepNumOfPoints");
		p.SetDefaultValue("360");
		p.SetDescription("The number of points for parameter sweep, valid when TypeOfSweep is set to be 'Number Of Points'");
	}

	{
		auto p = ADD_MODEL_PARAM(SweepStepSize);
		p.SetName("SweepStepSize");
		p.SetDefaultValue("1");
		p.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		p.SetDescription("The step size for parameter sweep, valid when TypeOfSweep is set to be 'Step Size'");
	}

	return true;
}
#endif

// ============================================================
// 构造函数
// ============================================================
RADAR_Tx_DBS_Measurement::RADAR_Tx_DBS_Measurement()
	: input()
	, AntPattern()
	, PRI(1e-4)
	, SamplingRate(10e6)
	, NumOfAntx(4)
	, NumOfAnty(4)
	, Dx(0.5)
	, Dy(0.5)
	, ParamToSweep(Sweep_Phi)
	, Theta_Phi(0.0)
	, TypeOfSweep(Linear_Number_of_Points)
	, SweepStart(0.0)
	, SweepStop(0.0)
	, SweepNumOfPoints(360)
	, SweepStepSize(0.0)
	, nAnt_(16)
	, inputRate_(1000)
	, sweepSamples_(360)
	, sweepStepRad_(0.0)
	, inputStartTime_(0.0)
	, inputTimeStep_(0.0)
{
	// 注意：SweepStop / SweepStepSize 在界面默认值为 360 deg / 1 deg。
	// 因为设置了 Units::ANGLE，进入 C++ 成员时通常已经是 radians。
	// 构造函数中的 0 不影响界面默认值，Setup 后会读取实际参数值。
}

// ============================================================
// 参数检查与派生量准备
// ============================================================
bool RADAR_Tx_DBS_Measurement::validateAndPrepare_()
{
	if (PRI <= 0.0)
	{
		POST_ERROR("PRI must be greater than 0.");
		return false;
	}

	if (SamplingRate <= 0.0)
	{
		POST_ERROR("SamplingRate must be greater than 0.");
		return false;
	}

	if (NumOfAntx < 1) NumOfAntx = 1;
	if (NumOfAnty < 1) NumOfAnty = 1;

	nAnt_ = NumOfAntx * NumOfAnty;
	if (nAnt_ < 1) nAnt_ = 1;

	inputRate_ = computeInputRate_();
	inputRate_ = clampInt(inputRate_, 1, 2147483647);

	sweepSamples_ = computeSweepSamples_();
	sweepSamples_ = clampInt(sweepSamples_, 1, 2147483647);

	sweepStepRad_ = computeSweepStepRad_(sweepSamples_);

	const size_t busSize = input.GetSize();
	if (busSize > 0)
	{
		inputStartTime_ = input[0].GetStartTime();
		inputTimeStep_ = input[0].GetTimeStep();
	}
	else
	{
		inputStartTime_ = 0.0;
		inputTimeStep_ = 0.0;
	}

	return true;
}

int RADAR_Tx_DBS_Measurement::computeInputRate_() const
{
	const double v = PRI * SamplingRate;
	if (!(v > 0.0) || !std::isfinite(v))
		return 1;

	// 默认 1e-4 * 10e6 = 1000。
	return static_cast<int>(std::floor(v + 0.5));
}

int RADAR_Tx_DBS_Measurement::computeSweepSamples_() const
{
	if (TypeOfSweep == Linear_Number_of_Points)
	{
		if (SweepNumOfPoints < 1)
			return 1;

		return SweepNumOfPoints;
	}

	// Linear:Step Size。
	// 帮助文档默认 SweepStart=0, SweepStop=360, SweepStepSize=1 时输出 360 点，
	// 因此这里采用不包含终点的点数计算：ceil(abs(stop-start)/abs(step))。
	const double span = SweepStop - SweepStart;
	const double stepAbs = std::abs(SweepStepSize);

	if (!(stepAbs > 0.0) || !std::isfinite(stepAbs))
		return 1;

	const double n = std::ceil(std::abs(span) / stepAbs);
	if (!(n > 0.0) || !std::isfinite(n))
		return 1;

	return static_cast<int>(n);
}

double RADAR_Tx_DBS_Measurement::computeSweepStepRad_(int nSamples) const
{
	if (nSamples <= 0)
		return 0.0;

	if (TypeOfSweep == Linear_Number_of_Points)
	{
		//   SweepStart=0, SweepStop=360deg, SweepNumOfPoints=360 时，
		//   内置输出对应角度为 0,1,2,...,359 deg；
		//   若按帮助表格中的 (Stop-Start+1)/N，会得到 1.002777...deg，
		//   与内置差值会在每个 sweep 内形成约 1deg 的锯齿/三角差值。
		//
		// 因此内置实际步长应为：
		//   Step = (SweepStop - SweepStart) / SweepNumOfPoints
		//
		// 主体步进采用该形式；最后一个点在 getThetaPhiForSweep_ 中钳到 SweepStop。
		const double span = SweepStop - SweepStart;
		return span / static_cast<double>(nSamples);
	}

	const double stepAbs = std::abs(SweepStepSize);
	if (!(stepAbs > 0.0) || !std::isfinite(stepAbs))
		return 0.0;

	return (SweepStop >= SweepStart) ? stepAbs : -stepAbs;
}

void RADAR_Tx_DBS_Measurement::applyInputRates_()
{
	const size_t busSize = input.GetSize();
	for (size_t k = 0; k < busSize; ++k)
	{
		input[k].SetRate(static_cast<unsigned>(inputRate_));
	}
}

ERESULT RADAR_Tx_DBS_Measurement::CalculateLatency()
{
	const size_t busSize = input.GetSize();
	if (busSize > 0)
	{
		inputStartTime_ = input[0].GetStartTime();
		inputTimeStep_ = input[0].GetTimeStep();
	}
	return (ERESULT)0;
}

bool RADAR_Tx_DBS_Measurement::Setup()
{
	if (!validateAndPrepare_())
		return false;

	applyInputRates_();

	// 帮助文档：Nsamples tokens are produced at the output port.
	AntPattern.SetRate(static_cast<unsigned>(sweepSamples_));

	return true;
}

bool RADAR_Tx_DBS_Measurement::Run()
{
	const size_t busSize = input.GetSize();

	// 驱动 TimedDFModel 时间轴，取第 0 路 bus 作为参考。
	if (busSize > 0)
	{
		(void)input[0].GetTime(0, GetCount());
		inputStartTime_ = input[0].GetStartTime();
		inputTimeStep_ = input[0].GetTimeStep();
	}

	const int nAntRead = std::min(nAnt_, static_cast<int>(busSize));

	for (int k = 0; k < sweepSamples_; ++k)
	{
		double thetaRad = 0.0;
		double phiRad = 0.0;
		double sweepAngleRad = 0.0;
		getThetaPhiForSweep_(k, thetaRad, phiRad, sweepAngleRad);

		Cx acc(0.0, 0.0);

		// 帮助文档公式在括号前有一个 1/(PRI*SamplingRate) 的平均项。
		//   PRI=1e-4, SamplingRate=1e6 => inputRate=100；
		//   3 路全 1 输入时，未平均得到 |300|^2=90000；
		//   内置输出为 |3|^2=9，说明内置先对时间采样求平均，再取平方。
		// 因此这里先对一个 PRI 中的所有时间采样点和所有阵元相干求和，
		// 再除以 inputRate_，最后取 abs(avg)^2。
		for (int i = 0; i < inputRate_; ++i)
		{
			for (int ky = 0; ky < NumOfAnty; ++ky)
			{
				for (int kx = 0; kx < NumOfAntx; ++kx)
				{
					const int antIndex = kx + NumOfAntx * ky;
					if (antIndex < 0 || antIndex >= nAntRead)
						continue;

					const double phase = computePhaseRad_(kx, ky, thetaRad, phiRad);
					const Cx rot = phaseRotator_(phase);

					acc += getInputCx_(antIndex, i) * rot;
				}
			}
		}

		Cx avg = acc;
		if (inputRate_ > 0)
		{
			avg /= static_cast<double>(inputRate_);
		}

		const double power = std::norm(avg);
		const Cx anglePoint(std::cos(sweepAngleRad), std::sin(sweepAngleRad));

		AntPattern[k] = power * anglePoint;
	}

	return true;
}

void RADAR_Tx_DBS_Measurement::getThetaPhiForSweep_(int k, double& thetaRad, double& phiRad, double& sweepAngleRad) const
{
	sweepAngleRad = SweepStart + static_cast<double>(k) * sweepStepRad_;

	// SweepStart=0, SweepStop=360deg, SweepNumOfPoints=360 时，
	// 内置在 Number of Points 模式下主体点使用 0,1,2,... 的步进，
	// 但最后一个输出点会被钳到 SweepStop。
	//
	// 对默认 0~360/360 点，即：
	//   k=0..358 使用 0..358deg；
	//   k=359 使用 360deg，而不是 359deg。
	//
	if (TypeOfSweep == Linear_Number_of_Points &&
		sweepSamples_ > 1 &&
		k == sweepSamples_ - 1)
	{
		sweepAngleRad = SweepStop;
	}

	if (ParamToSweep == Sweep_Phi)
	{
		thetaRad = Theta_Phi;
		phiRad = sweepAngleRad;
	}
	else
	{
		thetaRad = sweepAngleRad;
		phiRad = Theta_Phi;
	}

	thetaRad = clampFinite(thetaRad, 0.0);
	phiRad = clampFinite(phiRad, 0.0);
	sweepAngleRad = clampFinite(sweepAngleRad, 0.0);
}

double RADAR_Tx_DBS_Measurement::computePhaseRad_(int kx, int ky, double thetaRad, double phiRad) const
{
	// 参照 RADAR_PhaseShift 帮助文档：
	// phase = 2*pi * [kx*Dx*sin(theta)*cos(phi) + ky*Dy*sin(theta)*sin(phi)]
	const double sx = std::sin(thetaRad) * std::cos(phiRad);
	const double sy = std::sin(thetaRad) * std::sin(phiRad);

	const double cycles = static_cast<double>(kx) * Dx * sx
		+ static_cast<double>(ky) * Dy * sy;

	return 2.0 * M_PI * cycles;
}

RADAR_Tx_DBS_Measurement::Cx RADAR_Tx_DBS_Measurement::phaseRotator_(double phaseRad) const
{
	return Cx(std::cos(phaseRad), std::sin(phaseRad));
}

RADAR_Tx_DBS_Measurement::Cx RADAR_Tx_DBS_Measurement::getInputCx_(int antIndex, int sampleIndex)
{
	const size_t busSize = input.GetSize();
	if (antIndex < 0 || sampleIndex < 0)
		return Cx(0.0, 0.0);

	if (static_cast<size_t>(antIndex) >= busSize)
		return Cx(0.0, 0.0);

	// input[antIndex][sampleIndex] 为 EnvelopeSignal。
	// 注意：该函数不能声明为 const。
	// SystemVue 2020 中 EnvelopeCircularBufferBus 取出的 lane 在 const 上下文下
	// 会变成 const EnvelopeCircularBuffer，而该类型没有匹配的 const operator[]。
	const EnvSig x = input[static_cast<size_t>(antIndex)][sampleIndex];
	return x.complex();
}

double RADAR_Tx_DBS_Measurement::deg2rad(double x)
{
	return x * M_PI / 180.0;
}

double RADAR_Tx_DBS_Measurement::clampFinite(double x, double fallback)
{
	if (x != x)
		return fallback;

	if (!std::isfinite(x))
		return fallback;

	return x;
}

int RADAR_Tx_DBS_Measurement::clampInt(int x, int lo, int hi)
{
	if (x < lo) return lo;
	if (x > hi) return hi;
	return x;
}
