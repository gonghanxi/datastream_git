#include "AverageCx.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( AverageCx )
{	
	SET_MODEL_DESCRIPTION("Complex Averager");
	SET_MODEL_SYMBOL("SYM_Average");
	SET_MODEL_CATEGORY("Signal Processing");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(input);
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(NumInputsToAverage);
		param.SetDescription("Number of input blocks to average");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("8");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(BlockSize);
		param.SetDescription("Number of input blocks to average");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("1");
		param.SetUseDefault(1);
		param.SetSchematicDisplay(0);
	}
	return true;
}
#endif

AverageCx::AverageCx()
{
	
}

bool AverageCx::Setup()
{
	bool bStatus = true;
	if (NumInputsToAverage >= 1 && BlockSize >= 1)
	{
		input.SetRate(NumInputsToAverage*BlockSize);
		output.SetRate(BlockSize);
	}
	else
	{
		POST_ERROR("NumInputsToAverage and BlockSize must be greater than 0.");
        LOG_ERROR("NumInputsToAverage and BlockSize must be greater than 0.");
		bStatus = false;
	}

	return bStatus;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool AverageCx::Run()
{
	for (int n = 0; n < BlockSize; n++)
	{
		output[n] = 0.0;
		for (int i = 0; i < NumInputsToAverage; i++)
		{
			output[n] += input[i*BlockSize + n];
		}
		output[n] /= NumInputsToAverage;
	}
	return true;
}
