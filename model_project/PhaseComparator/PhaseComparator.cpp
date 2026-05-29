#include "PhaseComparator.h"

#include <cmath>
#include <complex>

static const double PC_PI = 3.14159265358979323846;
static const double PC_TWO_PI = 2.0 * PC_PI;

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(PhaseComparator)
{
	SET_MODEL_DESCRIPTION("Phase Comparator");
	SET_MODEL_SYMBOL("SYM_PhaseComparator");
	SET_MODEL_CATEGORY("Analog/RF");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(s1);
		port.SetName("s1");
		port.SetDescription("input signal 1");
	}
	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(s2);
		port.SetName("s2");
		port.SetDescription("input signal 2");
	}
	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
		port.SetName("output");
		port.SetDescription("output signal");
	}

	{
		SystemVueModelBuilder::DFParam enumParam =
			ADD_MODEL_ENUM_PARAM(PhaseCharacteristicType, PhaseCharacteristicTypeEnum);
		enumParam.SetName("PhaseCharacteristicType");
		enumParam.SetDescription("Type of analog phase comparator");
		enumParam.AddEnumeration("PhaseFreq", PhaseFreq);
		enumParam.AddEnumeration("Sinusoidal", Sinusoidal);
		enumParam.AddEnumeration("Triangular", Triangular);
		enumParam.SetDefaultValue("PhaseFreq");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(GainConstant);
		param.SetName("GainConstant");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("1");
		param.SetDescription("Small signal gain constant, in volts per degree");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(MaxAngle);
		param.SetName("MaxAngle");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("360");
		param.SetDescription(
			"Maximum unwrapped phase angle (+/- MaxAngle) for PhaseCharacteristicType = PhaseFreq");
	}

	return true;
}
#endif

PhaseComparator::PhaseComparator()
	: s1()
	, s2()
	, output()
	, PhaseCharacteristicType(PhaseFreq)
	, GainConstant(1.0)
	, MaxAngle(360.0)
	, fcOut_(0.0)
{
}

bool PhaseComparator::Setup()
{
	// 帮助文档说明：每次读 s1、s2 各 1 个 sample，输出 1 个 sample。
	// 这里显式设置 rate，解决第一个点没有正常 firing、保持默认 0 的问题。
	s1.SetRate(1U);
	s2.SetRate(1U);
	output.SetRate(1U);

	// 注意：
	// 这里不要调用 PropagateCharacterizationFrequency() 去检查 s1/s2 的 Fc。
	// Setup 阶段上游 CxToEnv 的 Fc 可能还没完全传播，过早检查会误报 fc=0。
	fcOut_ = 0.0;
	output.SetCharacterizationFrequency(fcOut_);

	return true;
}

ERESULT PhaseComparator::PropagateCharacterizationFrequency()
{
	// 内置 PhaseComparator 输出的是相位比较结果/控制电压，
	// 是 baseband/DC 实数控制量，不是 RF 包络本身。
	// 因此输出表征频率固定为 0。
	fcOut_ = 0.0;
	output.SetCharacterizationFrequency(fcOut_);

	// 不在这里硬性检查输入 Fc。
	// 否则在 SystemVue 传播顺序中可能会因为暂时读到 0 而误报。
	return true;
}

bool PhaseComparator::Initialize()
{
	if (!std::isfinite(GainConstant))
	{
		POST_ERROR("PhaseComparator: GainConstant must be finite.");
		return false;
	}

	if (!std::isfinite(MaxAngle) || MaxAngle < 0.0)
	{
		POST_ERROR("PhaseComparator: MaxAngle must be finite and >= 0.");
		return false;
	}

	return true;
}

double PhaseComparator::WrapToPi(double xRad)
{
	double x = std::fmod(xRad, PC_TWO_PI);
	if (x <= -PC_PI) x += PC_TWO_PI;
	if (x > PC_PI)  x -= PC_TWO_PI;
	return x;
}

double PhaseComparator::WrapDegreeSymmetric(double angleDeg, double maxAbsDeg)
{
	if (maxAbsDeg <= 0.0)
		return angleDeg;

	const double twoM = 2.0 * maxAbsDeg;
	double x = std::fmod(angleDeg + maxAbsDeg, twoM);
	if (x < 0.0) x += twoM;
	x -= maxAbsDeg;

	return x;
}

double PhaseComparator::TriangularPhase(double thetaRad)
{
	double x = WrapToPi(thetaRad);

	// 帮助文档图示：
	// triangular(-pi)   = 0
	// triangular(-pi/2) = -pi/2
	// triangular(0)     = 0
	// triangular(pi/2)  = pi/2
	// triangular(pi)    = 0
	if (x < -PC_PI / 2.0)
	{
		return -x - PC_PI;
	}
	else if (x <= PC_PI / 2.0)
	{
		return x;
	}
	else
	{
		return -x + PC_PI;
	}
}

bool PhaseComparator::Run()
{
	const double t = s1.GetTime(0, GetCount());

	double fc1 = s1.GetCharacterizationFrequency();
	double fc2 = s2.GetCharacterizationFrequency();

	if (!std::isfinite(fc1) || fc1 < 0.0)
		fc1 = 0.0;

	if (!std::isfinite(fc2) || fc2 < 0.0)
		fc2 = 0.0;

	const std::complex<double> x1 =
		s1[0].ConvertToNewFc(fc1, fc1, t);

	const std::complex<double> x2 =
		s2[0].ConvertToNewFc(fc2, fc1, t);

	// 内置帮助文档是分别定义 θ1(t)、θ2(t)，再做 θ1(t)-θ2(t)。
	// 不要用 angle(x1*conj(x2))，因为当 x2=0+j0 时乘积为 0，
	// 会导致第一个点输出 0，而内置会得到 theta2=0，输出 theta1。
	const double theta1 = std::atan2(x1.imag(), x1.real());
	const double theta2 = std::atan2(x2.imag(), x2.real());
	const double dTheta = WrapToPi(theta1 - theta2);

	const double scaleRad2VoltDeg = GainConstant * (180.0 / PC_PI);

	double outVal = 0.0;

	switch (PhaseCharacteristicType)
	{
	case PhaseFreq:
	{
		if (MaxAngle <= 0.0)
		{
			const double dThetaDeg = dTheta * (180.0 / PC_PI);
			outVal = GainConstant * dThetaDeg;
		}
		else
		{
			double dThetaDeg = dTheta * (180.0 / PC_PI);
			dThetaDeg = WrapDegreeSymmetric(dThetaDeg, MaxAngle);
			outVal = GainConstant * dThetaDeg;
		}
		break;
	}

	case Sinusoidal:
	{
		outVal = scaleRad2VoltDeg * std::sin(dTheta);
		break;
	}

	case Triangular:
	default:
	{
		const double triVal = TriangularPhase(dTheta);
		outVal = scaleRad2VoltDeg * triVal;
		break;
	}
	}

	output[0] = outVal;

	return true;
}