#include "CxToRect.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( CxToRect )
{	
	SET_MODEL_DESCRIPTION("Convert complex signal to real(I) and imaginary(Q) parts");
	SET_MODEL_SYMBOL("SYM_CxToRect");
	SET_MODEL_CATEGORY("Math Scalar");
	SET_MODEL_CATEGORY("Type Converters");

	ADD_MODEL_INPUT(Cx);
	ADD_MODEL_OUTPUT(Real);
	ADD_MODEL_OUTPUT(Imag);
	return true;
}
#endif

CxToRect::CxToRect()
{

}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool CxToRect::Run()
{
	Real[0] = Cx[0].real();
	Imag[0] = Cx[0].imag();
	return true;
}
