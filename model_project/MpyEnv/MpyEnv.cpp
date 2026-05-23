#include "MpyEnv.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(MpyEnv)
{
	SET_MODEL_DESCRIPTION("Envelope Signal Multiplier");
	SET_MODEL_SYMBOL("SYM_Mpy");
	SET_MODEL_CATEGORY("Analog/RF");
	SET_MODEL_CATEGORY("Beamforming");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(input);
		port.SetDescription("input signal");
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
		port.SetDescription("output signal");
	}

	{
		SystemVueModelBuilder::DFParam enumParam =
			ADD_MODEL_ENUM_PARAM(FcOut, SelectedFcOut);
		enumParam.SetDescription(
			"Output characterization frequency for the combined signal: Min, Max, Center, User defined");
		enumParam.AddEnumeration("min", min);
		enumParam.AddEnumeration("max", max);
		enumParam.AddEnumeration("center", center);
		enumParam.AddEnumeration("User defined", userDefined);
		enumParam.SetDefaultValue("2");
	}

	{
		SystemVueModelBuilder::DFParam param =
			ADD_MODEL_PARAMETER(UserDefinedFc);
		param.SetDescription("User defined output characterization frequency");
		param.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		param.SetDefaultValue("100e6");
		param.SetHideCondition("FcOut ~= 3");
	}

	return true;
}
#endif

MpyEnv::MpyEnv()
	: FcOut(center),
	UserDefinedFc(100e6),
	fc(0.0),
	fcmax(0.0),
	fcmin(0.0),
	fcmean(0.0),
	fcOut(0.0)
{
}

bool MpyEnv::Setup()
{
	(void)PropagateCharacterizationFrequency();
	output.SetRate(1U);   
	return true;
}

ERESULT MpyEnv::PropagateCharacterizationFrequency()
{
	bool bStatus = true;

	int ChannelNumIn = input.GetSize();
	if (ChannelNumIn <= 0) {
		fcOut = 0.0;
		output.SetCharacterizationFrequency(fcOut);
		return bStatus;
	}

	fcmax = 0.0;
	fcmean = 0.0;
	fcmin = input[0].GetCharacterizationFrequency();

	for (int i = 0; i < ChannelNumIn; ++i)
	{
		fc = input[i].GetCharacterizationFrequency();

		fcmax = (fcmax < fc ? fc : fcmax);
		fcmin = (fcmin > fc ? fc : fcmin);

		fcmean += fc;  
	}
	fcmean /= ChannelNumIn;     

	switch (FcOut)
	{
	case min:
		fcOut = fcmin;
		break;
	case max:
		fcOut = fcmax;
		break;
	case center:
		fcOut = fcmean;
		break;
	case userDefined:
		fcOut = UserDefinedFc;
		break;
	default:
		fcOut = fcmean;
		break;
	}

	output.SetCharacterizationFrequency(fcOut);

	return bStatus;
}

bool MpyEnv::Run()
{
	using SystemVueModelBuilder::EnvelopeSignal;

	int ChannelNumIn = input.GetSize();

	if (ChannelNumIn <= 0)
	{
		output[0] = EnvelopeSignal(0.0);
		return true;
	}

	double dTime = output.GetStartTime() + static_cast<double>(GetCount()) * output.GetTimeStep();

	fc = input[0].GetCharacterizationFrequency();
	EnvelopeSignal env0 = input[0][0].ConvertToNewFc(fc, fcOut, dTime);
	std::complex<double> prod = env0.complex();   

	for (int i = 1; i < ChannelNumIn; ++i)
	{
		fc = input[i].GetCharacterizationFrequency();
		EnvelopeSignal envi = input[i][0].ConvertToNewFc(fc, fcOut, dTime);
		prod *= envi.complex();
	}

	output[0] = EnvelopeSignal(prod);

	return true;
}

