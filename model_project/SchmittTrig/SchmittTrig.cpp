#include "SchmittTrig.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( SchmittTrig )
{	
	SET_MODEL_DESCRIPTION("Schmitt Trigger");
	SET_MODEL_SYMBOL("SYM_SchmittTrig");
	SET_MODEL_CATEGORY("Signal Processing");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(input);
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(ILow);
		param.SetDescription("Lower input trigger value");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("-1");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(IHigh);
		param.SetDescription("Higher input trigger value");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("1");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(OLow);
		param.SetDescription("Lower output trigger value");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("-1");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(OHigh);
		param.SetDescription("Higher output trigger value");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("1");
	}
	return true;
}
#endif

SchmittTrig::SchmittTrig()
{
	
}

bool SchmittTrig::Setup()
{
	bool bStatus = true;

	if (ILow > IHigh)
	{
		POST_ERROR("ILow must be smaller than IHigh.");
		bStatus = false;
	}

	if (OLow > OHigh)
	{
		POST_ERROR("OLow must be smaller than OHigh.");
		bStatus = false;
	}

	TrigStatus = 0;

	return bStatus;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool SchmittTrig::Run()
{
	if (input[0] > IHigh)
	{
		TrigStatus = 1;
	}

	if (input[0] < ILow)
	{
		TrigStatus = 0;
	}

	output[0] = TrigStatus ? OHigh : OLow;

	return true;
}
