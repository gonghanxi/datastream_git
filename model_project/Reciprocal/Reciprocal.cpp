#include "Reciprocal.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( Reciprocal )
{	
	SET_MODEL_DESCRIPTION("Reciprocal function");
	SET_MODEL_SYMBOL("SYM_Reciprocal");
	SET_MODEL_CATEGORY("Math Scalar");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(input);
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(MagLimit);
		param.SetDescription("Magnitude limit; non-zero value limits the output magnitude");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("0");
	}
	return true;
}
#endif

Reciprocal::Reciprocal()
{
	
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool Reciprocal::Run()
{
	if (MagLimit == 0)
	{
		output[0] = 1 / input[0];
	}
	
	if (MagLimit != 0 && input[0] == 0)
	{
		output[0] = MagLimit;
	}

	if (MagLimit != 0 && input[0] != 0)
	{
		if (1 / input[0] > MagLimit)
		{
			output[0] = MagLimit;
		}
		else if (1 / input[0] < -MagLimit)
		{
			output[0] = -MagLimit;
		}
		else
		{
			output[0] = 1 / input[0];
		}
	}
	return true;
}
