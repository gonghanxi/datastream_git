#include "GainFxp.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( GainFxp )
{	
	ADD_MODEL_INPUT( input );
	ADD_MODEL_OUTPUT( output );
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(Gain);
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("1");
		param.SetDescription("Gain value");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(FxpPos);
		param.SetDescription("Fixpoint position");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("4");
	}
	return true;
}
#endif

GainFxp::GainFxp()
{

}

bool GainFxp::Setup()
{
	bool bStatus = true;

	if (FxpPos < 0)
	{
		POST_ERROR("FxpPos must be >=0");
		bStatus = false;
	}

	return bStatus;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool GainFxp::Run()
{
	output[0] = Gain * input[0];
	return true;
}
