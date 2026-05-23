#include "CyclicShiftInt.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( CyclicShiftInt )
{	
	SET_MODEL_DESCRIPTION("Cyclically shift input data block with specified offset");
	SET_MODEL_SYMBOL("SYM_CyclicShift");
	SET_MODEL_CATEGORY("Signal Processing");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(input);
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(BlockSize);
		param.SetDescription("block size");
		param.SetDefaultValue("256");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(Offset);
		param.SetDescription("positive for shifting right and negative for shifting left (-BlockSize,BlockSize)");
		param.SetDefaultValue("0");
	}
	return true;
}
#endif

CyclicShiftInt::CyclicShiftInt()
{

}

bool CyclicShiftInt::Setup()
{
	bool bStatus = true;

	if (BlockSize > 0)
	{
		input.SetRate(BlockSize);
		output.SetRate(BlockSize);
	}
	else
	{
		POST_ERROR("BlockSize must be greater than 0.");
		bStatus = false;
	}

	if (Offset <= -BlockSize || Offset >= BlockSize)
	{
		POST_ERROR("Offset must be: -BlockSize < Offset < Blocksize");
		bStatus = false;
	}

	return bStatus;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool CyclicShiftInt::Run()
{
	for (int i = 0; i < BlockSize; i++)
	{
		int outIndex = Offset > 0 ? (i + Offset) % BlockSize : (i + Offset + BlockSize) % BlockSize;
		output[outIndex] = input[i];
	}
	return true;
}
