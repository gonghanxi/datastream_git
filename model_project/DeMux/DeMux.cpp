#include "DeMux.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( DeMux )
{	
	SET_MODEL_DESCRIPTION("Data Demultiplexer");
	SET_MODEL_SYMBOL("SYM_DeMux");
	SET_MODEL_CATEGORY("Routers/Resamplers");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(input);
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(control);
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
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

DeMux::DeMux()
{
	
}

bool DeMux::Setup()
{
	bool bStatus = true;

	if (BlockSize > 0)
	{
		input.SetRate(BlockSize);
		for (int i = 0; i < output.GetSize(); i++)
		{
			output[i].SetRate(BlockSize);
		}
	}
	else
	{
		POST_ERROR("BlockSize must be greater than 0.");
        LOG_ERROR("BlockSize must be greater than 0.");
		bStatus = false;
	}

	return bStatus;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool DeMux::Run()
{
	if (control[0] < 0 || control[0] + 1 > output.GetSize())
	{
		POST_ERROR("The control input can only accept values in the range [0, N - 1], where N is the output size.");
		return false;
	}

	for (int i = 0; i < BlockSize; i++)
	{
		output[control[0]][i] = input[i];
	}
	return true;
}
