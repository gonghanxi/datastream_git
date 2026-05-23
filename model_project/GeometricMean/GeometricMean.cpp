#include "GeometricMean.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( GeometricMean )
{	
	SET_MODEL_DESCRIPTION("Geometric Mean Function");
	SET_MODEL_SYMBOL("SYM_GeometricMean");
	SET_MODEL_CATEGORY("Signal Processing");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(input);
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(N);
		param.SetDescription("Number of samples in a block");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("10");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(Gain);
		param.SetDescription("Gain value");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("1");
	}
	return true;
}
#endif

GeometricMean::GeometricMean()
{

}

bool GeometricMean::Setup()
{
	bool bStatus = true;

	if (N > 0)
	{
		input.SetRate(N);
	}
	else
	{
		POST_ERROR("N must be greater than 0.");
        LOG_ERROR("N must be greater than 0.");
		bStatus = false;
	}

	return bStatus;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool GeometricMean::Run()
{
	double product = 1.0;
	for (int i = 0; i < N; i++)
	{
		product *= input[i];
	}
	output[0] = Gain * std::pow(product, 1.0 / N);
	return true;
}
