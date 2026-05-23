#include "Sinc.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( Sinc )
{	
	SET_MODEL_DESCRIPTION("Sinc Function");
	SET_MODEL_SYMBOL("SYM_Sinc");
	SET_MODEL_CATEGORY("Math Scalar");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(input);
		port.SetDescription("The input x to the sinc function.");
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
		port.SetDescription("The output of the sinc function.");
	}
	return true;
}
#endif

Sinc::Sinc()
{

}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool Sinc::Run()
{
	if (input[0] == 0)
	{
		output[0] = 1;
	}
	else
	{ 
		output[0] = sin(input[0]) / input[0];
	}
	return true;
}
