#include "PolarToCx_M.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( PolarToCx_M )
{	
	SET_MODEL_DESCRIPTION("Convert phase and magnitude to complex signal");
	SET_MODEL_SYMBOL("SYM_PolarToCx");
	SET_MODEL_CATEGORY("Matrix Scalar");
	SET_MODEL_CATEGORY("Type Converters");

	ADD_MODEL_INPUT(magnitude);
	ADD_MODEL_INPUT(phase);
	ADD_MODEL_OUTPUT(output);
	return true;
}
#endif

PolarToCx_M::PolarToCx_M()
{
	
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool PolarToCx_M::Run()
{
	int NRow = magnitude[0].NumRows();
	int NCol = magnitude[0].NumColumns();
	output[0].Resize(NRow, NCol);

	for (int row = 0; row < NRow; row++)
	{
		for (int col = 0; col < NCol; col++)
		{
			output[0](row, col) = std::polar(magnitude[0](row, col), phase[0](row, col));
		}
	}

	return true;
}
