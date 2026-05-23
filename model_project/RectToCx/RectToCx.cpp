#include "RectToCx.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( RectToCx )
{	
	SET_MODEL_DESCRIPTION("Convert real(I) and imaginary(Q) parts to complex signal");
	SET_MODEL_SYMBOL("SYM_RectToCx");
	SET_MODEL_CATEGORY("Math Scalar");
	SET_MODEL_CATEGORY("Type Converters");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(Real);
		port.SetOptional();
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(Imag);
		port.SetOptional();
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(Cx);
	}
	return true;
}
#endif

RectToCx::RectToCx()
{

}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool RectToCx::Run()
{
	Cx[0] = 0.0;

	if (Real.IsConnected())
	{
		Cx[0].real(Real[0]);
	}

	if (Imag.IsConnected())
	{
		Cx[0].imag(Imag[0]);
	}

	return true;
}
