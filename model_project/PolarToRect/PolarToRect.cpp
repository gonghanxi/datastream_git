#include "PolarToRect.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( PolarToRect )
{	
	SET_MODEL_DESCRIPTION("Convert phase and magnitude to real(I) and imaginary(Q) parts");
	SET_MODEL_SYMBOL("SYM_PolarToRect");
	SET_MODEL_CATEGORY("Math Scalar");
	SET_MODEL_CATEGORY("Type Converters");

	ADD_MODEL_INPUT(magnitude);
	ADD_MODEL_INPUT(phase);
	ADD_MODEL_OUTPUT(x);
	ADD_MODEL_OUTPUT(y);
	return true;
}
#endif

PolarToRect::PolarToRect()
{

}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool PolarToRect::Run()
{
	x[0] = magnitude[0] * std::cos(phase[0]);
	y[0] = magnitude[0] * std::sin(phase[0]);
	return true;
}
