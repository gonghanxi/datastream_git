#include "SubEnv.h"
#include <numeric>

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(SubEnv)
{
	SET_MODEL_DESCRIPTION("Envelope Signal Multiple Input Subtractor");
	SET_MODEL_SYMBOL("SYM_Sub");
	SET_MODEL_CATEGORY("Analog/RF");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(neg);
		port.SetDescription("input signals to subtract");
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(pos);
		port.SetDescription("input signal");
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
		port.SetDescription("output signal");
	}

	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(FcOut, SelectedFcOut);
		enumParam.SetDescription("Output characterization frequency for the combined signal: Min, Max, Center, User defined");
		enumParam.AddEnumeration("Min", min);
		enumParam.AddEnumeration("Max", max);
		enumParam.AddEnumeration("Center", center);
		enumParam.AddEnumeration("User defined", userDefined);
		enumParam.SetDefaultValue("2");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(UserDefinedFc);
		param.SetDescription("User defined output characterization frequency");
		param.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		param.SetDefaultValue("100e6");
		param.SetHideCondition("FcOut ~= 3");
	}
	return true;
}
#endif

SubEnv::SubEnv()
{
}

ERESULT SubEnv::PropagateCharacterizationFrequency()
{
	bool bStatus = true;

	fcmax = 0.0;
	fcmean = 0.0;
	fcmin = neg[0].GetCharacterizationFrequency();

	int ChannelNumNeg = neg.GetSize();
	for (int i = 0; i < ChannelNumNeg; i++)
	{
		fcNeg = neg[i].GetCharacterizationFrequency();

		fcmax = (fcmax < fcNeg ? fcNeg : fcmax);
		fcmin = (fcmin > fcNeg ? fcNeg : fcmin);
		fcmean += fcNeg;
	}

	fcmean /= ChannelNumNeg;

	switch (FcOut)
	{
		case min: fcOut = fcmin; break;
		case max: fcOut = fcmax; break;
		case center: fcOut = fcmean; break;
		case userDefined: fcOut = UserDefinedFc; break;
	}

	output.SetCharacterizationFrequency(fcOut);

	return bStatus;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool SubEnv::Run()
{
	double dTime = output.GetTime(0, GetCount());

	output[0] = 0.0;
	fcPos = pos.GetCharacterizationFrequency();
	output[0] += pos[0].ConvertToNewFc(fcPos, fcOut, dTime);

	int ChannelNumNeg = neg.GetSize();
	for (int i = 0; i < ChannelNumNeg; i++)
	{
		fcNeg = neg[i].GetCharacterizationFrequency();
		output[0] += -neg[i][0].ConvertToNewFc(fcNeg, fcOut, dTime);
	}

	return true;
}
