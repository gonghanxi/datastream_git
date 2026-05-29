#include "CxToPolar_M.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( CxToPolar_M )
{	
	SET_MODEL_DESCRIPTION("Convert complex signal to phase and magnitude");
	SET_MODEL_SYMBOL("SYM_CxToPolar");
	SET_MODEL_CATEGORY("Matrix Scalar");
	SET_MODEL_CATEGORY("Type Converters");

	ADD_MODEL_INPUT(input);
	ADD_MODEL_OUTPUT(magnitude);
	ADD_MODEL_OUTPUT(phase);
	return true;
}
#endif

CxToPolar_M::CxToPolar_M()
{

}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool CxToPolar_M::Run()
{
	int NRow = input[0].NumRows();
	int NCol = input[0].NumColumns();
	magnitude[0].Resize(NRow, NCol);
	phase[0].Resize(NRow, NCol);

	for (int row = 0; row < NRow; row++)
	{
		for (int col = 0; col < NCol; col++)
		{
			magnitude[0](row, col) = std::abs(input[0](row, col));
			phase[0](row, col) = std::arg(input[0](row, col));
		}
	}
	return true;
}
