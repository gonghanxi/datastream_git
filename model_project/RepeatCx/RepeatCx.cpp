#include "RepeatCx.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( RepeatCx )
{	
	SET_MODEL_DESCRIPTION("Data Repeater");
	SET_MODEL_SYMBOL("SYM_Repeat");
	SET_MODEL_CATEGORY("Routers/Resamplers");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(input);
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(NumTimes);
		param.SetDescription("Repetition factor");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("2");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(BlockSize);
		param.SetDescription("Number of data items in a block");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("1");
	}
	return true;
}
#endif

RepeatCx::RepeatCx()
{
	
}

bool RepeatCx::Setup()
{
	bool bStatus = true;

	if (NumTimes < 1)
	{
		POST_ERROR("NumTimes must be >= 1.");
		bStatus = false;
	}

	if (BlockSize < 1)
	{
		POST_ERROR("BlockSize must be >= 1.");
		bStatus = false;
	}

	input.SetRate(BlockSize);
	output.SetRate(BlockSize*NumTimes);

	return bStatus;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool RepeatCx::Run()
{
	for (int n = 0; n < NumTimes; n++)
	{
		for (int i = 0; i < BlockSize; i++)
		{
			output[n*BlockSize + i] = input[i];
		}
	}
	return true;
}
