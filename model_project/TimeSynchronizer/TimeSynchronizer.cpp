#include "TimeSynchronizer.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(TimeSynchronizer)
{
	SET_MODEL_DESCRIPTION("Synchronize signals in time");
	SET_MODEL_CATEGORY("Analog/RF");
	SET_MODEL_SYMBOL("SYM_TimeSynchronizer+Mode+");
	ADD_MODEL_INPUT(input);
	ADD_MODEL_OUTPUT(output);

	{
		SystemVueModelBuilder::DFParam e =
			ADD_MODEL_ENUM_PARAM(Mode, ModeEnum);
		e.AddEnumeration("ZeroPadding", TimeSynchronizer::ZeroPadding);
		e.AddEnumeration("TimeDelay", TimeSynchronizer::TimeDelay);
		e.SetDefaultValue("ZeroPadding");
		e.SetDescription("Time synchronization mode");
	}
	return true;
}
#endif

TimeSynchronizer::TimeSynchronizer() : Mode(ZeroPadding) {}

bool TimeSynchronizer::Setup()
{
	const int nin = input.GetSize();
	const int nout = output.GetSize();
	N_ = std::min(nin, nout);

	fifos_.assign(std::max(0, N_), {});
	lastValue_.assign(std::max(0, N_), 0.0);
	return true;
}

bool TimeSynchronizer::Run()
{
	if (N_ <= 0) return true;

	for (int i = 0; i < N_; ++i) {
		SampleD s;
		s.v = input[i][0U];            
		s.t = input[i].GetTime(0, 0);  
		fifos_[i].push_back(s);
	}

	double target = fifos_[0].front().t;
	for (int i = 1; i < N_; ++i) {
		const double t = fifos_[i].front().t;
		if (Mode == ZeroPadding) target = std::min(target, t);
		else                     target = std::max(target, t);
	}

	if (Mode == ZeroPadding) {
		for (int i = 0; i < N_; ++i) {
			double y = 0.0;
			if (!fifos_[i].empty() &&
				std::fabs(fifos_[i].front().t - target) <= eps()) {
				y = fifos_[i].front().v;
				lastValue_[i] = y;
				fifos_[i].pop_front();
			}
			output[i][0U] = y;
		}
	}
	else {
		for (int i = 0; i < N_; ++i) {
			while (!fifos_[i].empty() && fifos_[i].front().t <= target + eps()) {
				lastValue_[i] = fifos_[i].front().v;
				fifos_[i].pop_front();
			}
			output[i][0U] = lastValue_[i];
		}
	}

	return true;
}
