#include "EnvFcChange_M.h"
#include <cmath>

using namespace SystemVueModelBuilder;

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(EnvFcChange_M)
{
	SET_MODEL_DESCRIPTION("Envelope Signal Characterization Frequency Converter (Matrix)");
	SET_MODEL_SYMBOL("SYM_EnvFcChange");
	SET_MODEL_CATEGORY("Analog/RF");
	SET_MODEL_CATEGORY("Beamforming");

	{
		DFPort p = ADD_MODEL_INPUT(input);
		p.SetDescription("input signal (envelope matrix)");
	}
	{
		DFPort p = ADD_MODEL_OUTPUT(output);
		p.SetDescription("output signal (envelope matrix)");
	}

	{
		DFParam p = ADD_MODEL_PARAM(OutputFc);
		p.SetUnit(Units::FREQUENCY);
		p.SetDefaultValue("0");
		p.SetDescription("New characterization frequency");
	}

	{
		DFParam p = ADD_MODEL_PARAM(Bandwidth);
		p.SetUnit(Units::FREQUENCY);
		p.SetDefaultValue("0");
		p.SetDescription("Bandwidth of bandpass filter centered at OutputFc (used when input fc=0)");
	}

	return true;
}
#endif  

EnvFcChange_M::EnvFcChange_M()
	: OutputFc(0.0)
	, Bandwidth(0.0)
	, fc_in_(0.0)
	, fc_out_(0.0)
{
}

ERESULT EnvFcChange_M::PropagateCharacterizationFrequency()
{
	fc_in_ = input.GetCharacterizationFrequency();

	if (OutputFc > 0.0)
		fc_out_ = OutputFc;
	else
		fc_out_ = fc_in_;

	if (!std::isfinite(fc_out_) || fc_out_ < 0.0)
		fc_out_ = 0.0;

	output.SetCharacterizationFrequency(fc_out_);

	return true;
}

bool EnvFcChange_M::Setup()
{
	(void)PropagateCharacterizationFrequency();

	output.SetRate(1U);

	return true;
}

bool EnvFcChange_M::Run()
{
	const double t = output.GetTime(0, GetCount());

	const EnvelopeMatrix& xin = input[0];

	EnvelopeMatrix yout;
	yout.Resize(xin.NumRows(), xin.NumColumns());

	const double fc_in = fc_in_;
	const double fc_out = fc_out_;

	if (fc_in != fc_out)
	{
		for (size_t i = 0; i < xin.NumElements(); ++i)
		{
			const EnvelopeSignal& ein = xin(i);

			std::complex<double> cx_new =
				ein.ConvertToNewFc(fc_in, fc_out, t);

			CopyToEnvelopeSignal(cx_new, yout(i));
		}
	}
	else
	{
		for (size_t i = 0; i < xin.NumElements(); ++i)
		{
			yout(i) = xin(i);
		}
	}

	output[0] = yout;
	return true;
}
