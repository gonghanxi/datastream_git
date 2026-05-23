#include "Hermitian_M.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( Hermitian_M )
{	
	SET_MODEL_DESCRIPTION("Hermitian Matrix Function");
	SET_MODEL_SYMBOL("SYM_Hermitian_M");
	SET_MODEL_CATEGORY("Math Matrix");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(input);
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
	}
	return true;
}
#endif

Hermitian_M::Hermitian_M()
{

}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool Hermitian_M::Run()
{
	int outRows = input[0].NumColumns();
	int outCols = input[0].NumRows();
	output[0].Resize(outRows, outCols);

	for (int m = 0; m < outRows; m++)
	{
		for (int n = 0; n < outCols; n++)
		{
			output[0](m, n) = std::conj(input[0](n, m));
		}
	}
	return true;
}
