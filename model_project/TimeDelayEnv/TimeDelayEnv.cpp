#include "TimeDelayEnv.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(TimeDelayEnv)
{
	SET_MODEL_DESCRIPTION("Ideal Time Delay Block (Envelope), Delays the signal for a certain amount of time.");
	SET_MODEL_SYMBOL("SYM_TimeDelayEnv");
	SET_MODEL_CATEGORY("Signal Processing");

	ADD_MODEL_INPUT(input);
	ADD_MODEL_OUTPUT(output);

	{
		SystemVueModelBuilder::DFParam p =
			ADD_MODEL_ENUM_PARAM(Unit, UnitEnum);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.AddEnumeration("Time", Unit_Time);
		p.AddEnumeration("TimeStep", Unit_TimeStep);
		p.SetDefaultValue("Time");
		p.SetDescription("Time delay unit: Time, TimeStep");
	}

	{
		SystemVueModelBuilder::DFParam p =
			ADD_MODEL_PARAM(T);
		p.SetUnit(SystemVueModelBuilder::Units::TIME);
		p.SetDefaultValue("0");
		p.SetDescription("Delay in time");
		p.SetHideCondition("Unit ~= 0");
	}

	{
		SystemVueModelBuilder::DFParam p =
			ADD_MODEL_PARAM(N);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("0");
		p.SetDescription("Delay in number of time steps");
		p.SetHideCondition("Unit ~= 1");
	}

	return true;
}
#endif

TimeDelayEnv::TimeDelayEnv()
	: Unit(Unit_Time),
	T(0.0),
	N(0),
	delaySeconds_(0.0)
{
}

bool TimeDelayEnv::Setup()
{
	input.SetRate(1U);
	output.SetRate(1U);

	if (T < 0.0) {
		POST_ERROR("TimeDelayEnv: T must be >= 0.");
		return false;
	}
	if (N < 0) {
		POST_ERROR("TimeDelayEnv: N must be >= 0.");
		return false;
	}

	delaySeconds_ = 0.0;
	return true;
}

ERESULT TimeDelayEnv::CalculateLatency()
{
	const double fsIn = input.GetSampleRate();
	const double dtIn = input.GetTimeStep();
	const double t0 = input.GetStartTime();

	if (Unit == Unit_Time) {
		delaySeconds_ = T;
	}
	else {
		if (dtIn <= 0.0) {
			POST_ERROR("TimeDelayEnv: input signal must be timed (sample rate > 0).");
			return -1;
		}
		delaySeconds_ = static_cast<double>(N) * dtIn;
	}

	if (fsIn > 0.0) {
		output.SetSampleRate(fsIn);
		output.SetStartTime(t0 + delaySeconds_);
	}

	const double fcIn = input.GetCharacterizationFrequency();
	output.SetCharacterizationFrequency(fcIn);

	return 0;
}

bool TimeDelayEnv::Run()
{
	output[0U] = input[0U];

	Advance();
	return true;
}
