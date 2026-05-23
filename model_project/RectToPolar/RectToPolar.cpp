#include "RectToPolar.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( RectToPolar )
{	
	SET_MODEL_DESCRIPTION("Convert real(I) and imaginary(Q) parts to phase and magnitude");
	SET_MODEL_SYMBOL("SYM_RectToPolar");
	SET_MODEL_CATEGORY("Math Scalar");
	SET_MODEL_CATEGORY("Type Converters");

	ADD_MODEL_INPUT(x);
	ADD_MODEL_INPUT(y);
	ADD_MODEL_OUTPUT(magnitude);
	ADD_MODEL_OUTPUT(phase);
	return true;
}
#endif

RectToPolar::RectToPolar()
{

}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool RectToPolar::Run()
{
	magnitude[0] = std::sqrt(std::pow(x[0], 2) + std::pow(y[0], 2));
	phase[0] = std::atan2(x[0], y[0]);
	return true;
}
