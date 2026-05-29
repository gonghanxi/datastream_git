#include "CxToRect_M.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( CxToRect_M )
{	
	SET_MODEL_DESCRIPTION("Convert complex signal to real(I) and imaginary(Q) parts");
	SET_MODEL_SYMBOL("SYM_CxToRect");
	SET_MODEL_CATEGORY("Matrix Scalar");
	SET_MODEL_CATEGORY("Type Converters");

	ADD_MODEL_INPUT(input);
	ADD_MODEL_OUTPUT(real);
	ADD_MODEL_OUTPUT(imag);
	return true;
}
#endif

CxToRect_M::CxToRect_M()
{
	
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool CxToRect_M::Run()
{
	int NRow = input[0].NumRows();
	int NCol = input[0].NumColumns();
	real[0].Resize(NRow, NCol);
	imag[0].Resize(NRow, NCol);

	for (int row = 0; row < NRow; row++)
	{
		for (int col = 0; col < NCol; col++)
		{
			real[0](row, col) = input[0](row, col).real();
            imag[0](row, col) = input[0](row, col).imag();
		}
	}

	return true;
}
