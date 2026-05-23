#include "Variance.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( Variance )
{	
	SET_MODEL_DESCRIPTION("Variance Function");
	SET_MODEL_SYMBOL("SYM_Variance");
	SET_MODEL_CATEGORY("Signal Processing");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(in);
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(mean);
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(variance);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(BlockSize);
		param.SetDescription("Number of inputs to process between each mean and variance estimate");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("1");
	}
	return true;
}
#endif

Variance::Variance()
{
	sum = 0.0;
	sumSqr = 0.0;
	sumN = 0;
}

bool Variance::Setup()
{
	bool bStatus = true;
	if (BlockSize > 0)
	{
		in.SetRate(BlockSize);
	}
	else
	{
		POST_ERROR("BlockSize must be greater than 0.");
		bStatus = false;
	}
	return bStatus;
}

bool Variance::Run()
{
	sumN += BlockSize;

	for (int i = 0; i < BlockSize; i++)
	{
		sum += in[i];
		sumSqr += in[i] * in[i];
	}
	mean[0] = sum / sumN;
	variance[0] = sumSqr / sumN - mean[0] * mean[0];
	return true;
}
