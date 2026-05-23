#include "ReverseEnv.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( ReverseEnv )
{	
	SET_MODEL_DESCRIPTION("Data Reverser for Complex");
	SET_MODEL_SYMBOL("SYM_Reverse");
	SET_MODEL_CATEGORY("Signal Processing");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(input);
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(N);
		param.SetDefaultValue("64");
	}
	return true;
}
#endif

ReverseEnv::ReverseEnv()
{

}

bool ReverseEnv::Setup()
{
	bool bStatus = true;

	if (N > 0)
	{
		input.SetRate(N);
		output.SetRate(N);
	}

	else
	{
		POST_ERROR("Port rate must be greater than 0.");
		bStatus = false;
	}

	return bStatus;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool ReverseEnv::Run()
{
	for (int i = 0; i < N; i++)
	{
		output[N - i - 1] = input[i];
	}
	return true;
}
