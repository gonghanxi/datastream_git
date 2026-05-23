#include "CxToPolar.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( CxToPolar )
{	
	SET_MODEL_DESCRIPTION("Convert complex signal to phase and magnitude");
	SET_MODEL_SYMBOL("SYM_CxToPolar");
	SET_MODEL_CATEGORY("Math Scalar");
	SET_MODEL_CATEGORY("Type Converters");

	ADD_MODEL_INPUT(input);
	ADD_MODEL_OUTPUT(magnitude);
	ADD_MODEL_OUTPUT(phase);
	return true;
}
#endif

CxToPolar::CxToPolar()
{

}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool CxToPolar::Run()
{
	magnitude[0] = std::abs(input[0]);
	phase[0] = std::arg(input[0]);
	return true;
}
