#include "SampleMean_M.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( SampleMean_M )
{	
	SET_MODEL_DESCRIPTION("Matrix Mean Value");
	SET_MODEL_SYMBOL("SYM_SampleMean_M");
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

SampleMean_M::SampleMean_M()
{
	
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool SampleMean_M::Run()
{
	int N = input[0].NumElements();
	double sum = 0;
	for (int i = 0; i < N; i++)
	{
		sum += input[0](i);
	}
	output[0] = sum / N;
	return true;
}
