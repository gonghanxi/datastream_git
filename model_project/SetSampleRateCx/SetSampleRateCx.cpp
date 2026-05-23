#include "SetSampleRateCx.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(SetSampleRateCx)
{
	SET_MODEL_DESCRIPTION("Custom Set Sample Rate( Datatype: double )");
	SET_MODEL_SYMBOL("SYM_SetSampleRate");
	SET_MODEL_CATEGORY("Routers/Resamplers");

	ADD_MODEL_INPUT(input);
	ADD_MODEL_OUTPUT(output);

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(SampleRate);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("Sample_Rate");
		p.SetDescription("Sample rate");  
	}

	return true;
}
#endif

SetSampleRateCx::SetSampleRateCx()
	: SampleRate(0.0)
{}

bool SetSampleRateCx::Setup()
{
	input.SetRate(1U);
	output.SetRate(1U);

	if (!(SampleRate > 0.0)) {
		const char* msg =
			"SetSampleRate_self: Please enter a positive SampleRate (e.g., 1e6). "
			"The default placeholder 'Sample_Rate' is not a real value.";
		POST_ERROR(msg);
		return false;
	}
	const double fs = SampleRate;

	const double fsIn = input.GetSampleRate(); 
	if (fsIn > 0.0 && !almost_equal(fsIn, fs)) {
		const std::string msg = "SetSampleRate_self: input is already timed at " +
			std::to_string(fsIn) + " Hz, but SampleRate = " +
			std::to_string(fs) +
			" Hz. Use a resampler to change rate, or match SampleRate.";
		POST_ERROR(msg.c_str());
		return false;
        }

        output.SetSampleRate(fs);

	return true;
}

bool SetSampleRateCx::Run()
{
	output[0U] = input[0U];
	Advance();
	return true;
}

