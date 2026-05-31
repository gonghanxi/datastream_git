#include "Latch.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( Latch )
{	
	SET_MODEL_DESCRIPTION("Data Latch with Clock Control");
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

Latch::Latch()
{
	
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool Latch::Run()
{
	if (clock[0] != 0)
	{
		storedValue = input[0];
		output[0] = input[0];
	}
	else
	{
		output[0] = storedValue;
	}
	return true;
}
