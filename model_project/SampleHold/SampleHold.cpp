#include "SampleHold.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( SampleHold )
{	
	SET_MODEL_DESCRIPTION("Sample and hold with clock control");
	SET_MODEL_SYMBOL("SYM_SampleHold");
	SET_MODEL_CATEGORY("Routers/Resamplers");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(input);
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(clock);
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
	}
	return true;
}
#endif

SampleHold::SampleHold()
{

}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool SampleHold::Run()
{
	if (clock[0] != 0)
	{
		output[0] = input[0];
	}
	return true;
}
