#include "PhaseComparator.h"

#include <cmath>

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
		port.SetDescription("input signal 1 (envelope)");
	}
	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(s2);
		port.SetName("s2");
		port.SetDescription("input signal 2 (envelope)");
	}
	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
		port.SetName("output");
		port.SetDescription("output signal (envelope)");
	}

	{
		SystemVueModelBuilder::DFParam enumParam =
			ADD_MODEL_ENUM_PARAM(PhaseCharacteristicType, PhaseCharacteristicTypeEnum);
		enumParam.SetDescription("Type of analog phase comparator: PhaseFreq, Sinusoidal, Triangular");
		enumParam.AddEnumeration("PhaseFreq", PhaseFreq);
		enumParam.AddEnumeration("Sinusoidal", Sinusoidal);
		enumParam.AddEnumeration("Triangular", Triangular);
		enumParam.SetDefaultValue("0");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(GainConstant);
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("1");
		param.SetDescription("Small signal gain constant, in volts per degree");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(MaxAngle);
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

ERESULT PhaseComparator::PropagateCharacterizationFrequency()
{
	bool bStatus = true;

	const double fc1 = s1.GetCharacterizationFrequency();
	const double fc2 = s2.GetCharacterizationFrequency();

	if (fc1 <= 0.0 || fc2 <= 0.0)
	{
		POST_ERROR("PhaseComparator: inputs must be envelope signals with characterization frequency > 0.");
		bStatus = false;
	}

	fcOut_ = fc1;
	output.SetCharacterizationFrequency(fcOut_);

	return bStatus;
}

bool PhaseComparator::Initialize()
{
	if (!std::isfinite(GainConstant))
	{
		POST_ERROR("PhaseComparator: GainConstant must be finite.");
		return false;
	}

	if (MaxAngle < 0.0)
	{
		POST_ERROR("PhaseComparator: MaxAngle must be >= 0.");
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

	if (x < 0.0)                  
	{
		return 0.5 * x;
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
	const double t = output.GetTime(0, GetCount());

	const double fc1 = s1.GetCharacterizationFrequency();
	const double fc2 = s2.GetCharacterizationFrequency();
	const double fcOut = fcOut_;   

	const std::complex<double> x1 =
		s1[0].ConvertToNewFc(fc1, fcOut, t);
	const std::complex<double> x2 =
		s2[0].ConvertToNewFc(fc2, fcOut, t);

	const std::complex<double> z = x1 * std::conj(x2);
	const double dTheta = std::atan2(z.imag(), z.real());   

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

	output[0] = std::complex<double>(outVal, 0.0);

	return true;
}
