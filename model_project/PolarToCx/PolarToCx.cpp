#include "PolarToCx.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( PolarToCx )
{	
	SET_MODEL_DESCRIPTION("Convert phase and magnitude to complex signal");
	SET_MODEL_SYMBOL("SYM_PolarToCx");
	SET_MODEL_CATEGORY("Math Scalar");
	SET_MODEL_CATEGORY("Type Converters");

	ADD_MODEL_INPUT(magnitude);
	ADD_MODEL_INPUT(phase);
	ADD_MODEL_OUTPUT(output);
	return true;
}
#endif

PolarToCx::PolarToCx()
{

}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool PolarToCx::Run()
{
	output[0] = std::polar(magnitude[0], phase[0]);
	return true;
}
