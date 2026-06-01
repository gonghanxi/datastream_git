#include "DeadZone.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( DeadZone )
{	
	SET_MODEL_DESCRIPTION("Dead Zone Non-linearity");
	SET_MODEL_SYMBOL("SYM_DeadZone");
	SET_MODEL_CATEGORY("Signal Processing");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(input);
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(K);
		param.SetDescription("Magnitude gain");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("1");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(Low);
		param.SetDescription("Lower dead zone value");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("0");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(High);
		param.SetDescription("Higher dead zone value");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("1");
	}
	return true;
}
#endif

DeadZone::DeadZone()
{

}

bool DeadZone::Setup()
{
	bool bStatus = true;

	if (Low >= High)
	{
		POST_ERROR("Low must be smaller than High.");
	}

	return bStatus;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool DeadZone::Run()
{
	if (input[0] > High)
	{
		output[0] = K * (input[0] - High);
	}

	else if (input[0] < Low)
	{
		output[0] = K * (input[0] - Low);
	}

	else
	{
		output[0] = 0;
	}
	return true;
}
