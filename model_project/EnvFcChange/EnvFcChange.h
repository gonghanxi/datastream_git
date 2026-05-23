#pragma once
#include "ModelBuilder.h"
#include "TimedDFModel.h"
#include "TimedCircularBuffer.h"
#include "EnvelopeSignal.h"
#include "SystemVue.h"
#include <complex>
#include <cmath>
#include <limits>

class SYSTEMVUEMODELBUILDER_API EnvFcChange : public SystemVueModelBuilder::TimedDFModel
{
public:
	static constexpr double kPI = 3.14159265358979323846;

	DECLARE_MODEL_INTERFACE(EnvFcChange);
	EnvFcChange();

	bool Setup() override;
	bool Run() override;
	ERESULT PropagateCharacterizationFrequency() override;

	SystemVueModelBuilder::EnvelopeCircularBuffer input;
	SystemVueModelBuilder::EnvelopeCircularBuffer output;

	double OutputFc;
	double Bandwidth;

private:
	double Ts_{ 0.0 };
	double lastTime_{ std::numeric_limits<double>::quiet_NaN() };
	const double kTsAlpha_{ 0.02 };

	double alpha_{ 0.0 };
	double i_lp_{ 0.0 };
	double q_lp_{ 0.0 };

	static inline double clip(double x, double lo, double hi)
	{
		return (x < lo ? lo : (x > hi ? hi : x));
	}
};
