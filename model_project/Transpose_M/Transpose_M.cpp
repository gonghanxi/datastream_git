#include "Transpose_M.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( Transpose_M )
{	
	SET_MODEL_DESCRIPTION("Transpose Matrix Converter");
	SET_MODEL_SYMBOL("SYM_Transpose_M");
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

Transpose_M::Transpose_M()
{

}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool Transpose_M::Run()
{
	int M = input[0].NumRows();
	int N = input[0].NumColumns();
	output[0].Resize(N, M);
	for (int m = 0; m < M; m++)
	{
		for (int n = 0; n < N; n++)
		{
			output[0](n, m) = input[0](m, n);
		}
	}
	return true;
}
