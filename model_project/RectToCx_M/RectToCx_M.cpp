#include "RectToCx_M.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( RectToCx_M )
{	
	SET_MODEL_DESCRIPTION("Convert real(I) and imaginary(Q) parts to complex signal");
	SET_MODEL_SYMBOL("SYM_RectToCx");
	SET_MODEL_CATEGORY("Matrix Scalar");
	SET_MODEL_CATEGORY("Type Converters");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(real);
		port.SetOptional();
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(imag);
		port.SetOptional();
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
	}
	return true;
}
#endif

RectToCx_M::RectToCx_M()
{
	
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool RectToCx_M::Run()
{
	int NRow = real.IsConnected() ? real[0].NumRows() : 1;
	int NCol = real.IsConnected() ? real[0].NumColumns() : 1;
	output[0].Resize(NRow, NCol);

	for (int row = 0; row < NRow; row++)
	{
		for (int col = 0; col < NCol; col++)
		{
			output[0](row, col) = 0.0;

			if (real.IsConnected())
			{
				output[0](row, col).real(real[0](row, col));
			}

			if (imag.IsConnected())
			{
				output[0](row, col).imag(imag[0](row, col));
			}
		}
	}
	return true;
}
