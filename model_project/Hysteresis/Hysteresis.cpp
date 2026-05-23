#include "Hysteresis.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( Hysteresis )
{	
	SET_MODEL_DESCRIPTION("Hysteresis Function");
	SET_MODEL_SYMBOL("SYM_Hysteresis");
	SET_MODEL_CATEGORY("Signal Processing");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(input);
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(Bandwidth);
		param.SetDescription("Rate specified damping bandwidth");
		param.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		param.SetDefaultValue("0");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(Backlash);
		param.SetDescription("Backlash threshold");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("0");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(Gain);
		param.SetDescription("Backlash gain");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("1");
	}
	return true;
}
#endif

Hysteresis::Hysteresis()
{
	
}

bool Hysteresis::Setup()
{
	bool bStatus = true;

	InternalState = 0;

	if (Backlash < 0)
	{
		POST_ERROR("Bandwidth must not be negtive.");
        LOG_ERROR("Bandwidth must not be negtive.");
		bStatus = false;
	}

	return bStatus;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool Hysteresis::Run()
{
	bool bStatus = true;

	SampleRate = input.GetSampleRate();

	if (Bandwidth < 0 || Bandwidth > SampleRate)
	{
		POST_ERROR("Bandwidth should be: 0 <= Bandwidth <= SampleRate");
		bStatus = false;
	}

	Difference = (input[0] - InternalState);

	if (std::abs(Difference) > Backlash)
	{
		InternalState += (Difference - Backlash * Difference / std::abs(Difference)) * Bandwidth / SampleRate;
	}
	
	output[0] = InternalState * Gain;

	return bStatus;
}
