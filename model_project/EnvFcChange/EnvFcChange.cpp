#include "EnvFcChange.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(EnvFcChange)
{
	SET_MODEL_DESCRIPTION("Envelope Signal Characterization Frequency Converter");
	SET_MODEL_SYMBOL("SYM_EnvFcChange");
	SET_MODEL_CATEGORY("Analog/RF");

	{ auto p = ADD_MODEL_INPUT(input);   p.SetDescription("input signal (envelope)"); }
	{ auto p = ADD_MODEL_OUTPUT(output); p.SetDescription("output signal (envelope)"); }

	{ auto p = ADD_MODEL_PARAM(OutputFc);
	p.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
	p.SetDefaultValue("0");
	p.SetDescription("New characterization frequency"); }

	{ auto p = ADD_MODEL_PARAM(Bandwidth);
	p.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
	p.SetDefaultValue("0");
	p.SetDescription("Bandwidth for LPF when input fc=0 (0->use OutputFc)"); }

	return true;
}
#endif

EnvFcChange::EnvFcChange()
	: OutputFc(0.0), Bandwidth(0.0)
{
}

ERESULT EnvFcChange::PropagateCharacterizationFrequency()
{
	output.SetCharacterizationFrequency(OutputFc);
	return true;
}

bool EnvFcChange::Setup()
{
	input.SetRate(1U);
	output.SetRate(1U);

	(void)PropagateCharacterizationFrequency();

	Ts_ = 0.0;
	lastTime_ = std::numeric_limits<double>::quiet_NaN();
	alpha_ = 0.0;
	i_lp_ = q_lp_ = 0.0;
	return true;
}

bool EnvFcChange::Run()
{
	using namespace SystemVueModelBuilder;

	const EnvelopeSignal x = input[0U];

	const double tOut = output.GetStartTime() + static_cast<double>(GetCount()) * output.GetTimeStep();

	const double f1 = input.GetCharacterizationFrequency();
	const double f2 = OutputFc;

	if (output.GetCharacterizationFrequency() != f2)
		output.SetCharacterizationFrequency(f2);

	if (f1 > 0.0) {
		output[0U] = 0;
		output[0U] += x.ConvertToNewFc(f1, f2, tOut);
	}
	else {
		if (f2 <= 0.0) {
			output[0U] = x.real();
		}
		else {
			if (Ts_ > 0.0) {
				double BW = (Bandwidth > 0.0 ? Bandwidth : f2);
				if (BW > 0.0) {
					const double fc_lp = 0.5 * BW;
					const double a = std::exp(-2.0 * kPI * fc_lp * Ts_);
					alpha_ = clip(a, 0.0, 0.999999);
				}
				else {
					alpha_ = 0.0;
				}
			}
			const double xr = x.real();
			const double c = std::cos(2.0 * kPI * f2 * tOut);
			const double s = std::sin(2.0 * kPI * f2 * tOut);

			const double i_raw = xr * c;
			const double q_raw = xr * (-s);

			const double one_ma = 1.0 - alpha_;
			i_lp_ = one_ma * i_raw + alpha_ * i_lp_;
			q_lp_ = one_ma * q_raw + alpha_ * q_lp_;

			output[0U] = 0;
			output[0U] += std::complex<double>(i_lp_, q_lp_);
		}
	}
	return true;
}

