#include "AverageCxWOffset.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( AverageCxWOffset )
{	
	SET_MODEL_DESCRIPTION("Complex Averager with Offset Control");
	SET_MODEL_SYMBOL("SYM_AverageCxWOffset");
	SET_MODEL_CATEGORY("Signal Processing");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(input);
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(Offset);
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(NumInputsToAverage);
		param.SetDescription("Number of input blocks to average");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("256");
	}
	return true;
}
#endif

AverageCxWOffset::AverageCxWOffset()
{
	
}

bool AverageCxWOffset::Setup()
{
	bool bStatus = true;

	initialZeros = 0;
	currentSum = 0;
	currentAverage = 0;
	bufferIndex = 0;

	return bStatus;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool AverageCxWOffset::Run()
{	
	bool bStatus = true;
	if (Offset[0] <= 0)
	{
		POST_ERROR(" The offset must be non-negative.");
		bStatus = false;
	}

	if (initialZeros < Offset[0])
	{
		output[0] = 0;
		initialZeros++;
	}
	else
	{
		bufferIndex++;

		if (bufferIndex > NumInputsToAverage)
		{
			currentAverage = currentSum / double(NumInputsToAverage);
			bufferIndex -= NumInputsToAverage;
			currentSum = 0;
		}
		currentSum += input[0];
		output[0] = currentAverage;
	}
	return bStatus;
}
