#include "AddEnv_M.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( AddEnv_M )
{	
	SET_MODEL_DESCRIPTION("Envelope Matrix Signal Adder");
	SET_MODEL_SYMBOL("SYM_AddEnv");
	SET_MODEL_CATEGORY("Beamforming");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(input);
		port.SetDescription("input signal");
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
		port.SetDescription("output signal");
	}
	return true;
}
#endif

AddEnv_M::AddEnv_M()
{
	
}

ERESULT AddEnv_M::PropagateCharacterizationFrequency()
{
	bool bStatus = true;

	// 所有通道的特征频率都需要一致
	ChannelNum = input.GetSize();
	double currentFc = input[0].GetCharacterizationFrequency();
	for (int i = 0; i < ChannelNum; i++)
	{
		if (currentFc != input[i].GetCharacterizationFrequency())
		{
			POST_ERROR("All input envelope matrices must have the same characterization frequency.");
			bStatus = false;
		}
		currentFc = input[i].GetCharacterizationFrequency();
	}

	return bStatus;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool AddEnv_M::Run()
{
	
	int NRow = input[0][0].NumRows();
	int NCol = input[0][0].NumColumns();

	output[0].Resize(NRow, NCol);
	output[0].Zero();
	for (int i = 0; i < ChannelNum; i++)
	{
		output[0] += input[i][0];
	}
		
	return true;
}
