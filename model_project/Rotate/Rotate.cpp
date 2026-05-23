#include "Rotate.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( Rotate )
{	
	SET_MODEL_DESCRIPTION("Complex Rotate Function");
	SET_MODEL_SYMBOL("SYM_Rotate");
	SET_MODEL_CATEGORY("Math Scalar");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(input);
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(RotationAngle);
		param.SetDescription("Magnitude limit; non-zero value limits the output magnitude");
		param.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		param.SetDefaultValue("0");
	}
	return true;
}
#endif

Rotate::Rotate()
{
	
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool Rotate::Run()
{
	const std::complex<double> j(0.0, 1.0);
	output[0] = input[0] * std::exp(j*RotationAngle);
	return true;
}
