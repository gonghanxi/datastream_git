#include "RADAR_SummerBusRF.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( RADAR_SummerBusRF )
{	
	SET_MODEL_DESCRIPTION("RF signal summer");

	SET_MODEL_CATEGORY("Signal Processing");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(input1);
		port.SetName("IN1");
		port.SetDescription("The first input");
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(input2);
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
	}

	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(FcOut, SelectedFcOut);
		enumParam.AddEnumeration("min", min);
		enumParam.AddEnumeration("center", center);
		enumParam.AddEnumeration("max", max);
		enumParam.SetDefaultValue("2");
	}
	return true;
}
#endif

RADAR_SummerBusRF::RADAR_SummerBusRF()
{

}

//-----------------------------------------------------------------------------------
//	Characterization frequency propagate
//		Unify the characterization frequency.
//-----------------------------------------------------------------------------------

ERESULT RADAR_SummerBusRF::PropagateCharacterizationFrequency()
{
	bool bStatus = true;

	int	ChannelNumIn1 = input1.GetSize();
	int	ChannelNumIn2 = input2.GetSize();
	int	ChannelNumOut = output.GetSize();

	// 两个输入以及输出端口的通道数需要一致
	if (ChannelNumIn1 != ChannelNumIn2)
	{
		POST_ERROR("The width of input1 and input2 should be the same.");
		bStatus = false;
	}

	if (ChannelNumIn1 != ChannelNumOut)
	{
		POST_ERROR("The width of input and output should be the same.");
		bStatus = false;
	}

	for (int i = 0; i < ChannelNumOut; i++)
	{
		fc1 = input1[i].GetCharacterizationFrequency();
		fc2 = input2[i].GetCharacterizationFrequency();

		// 统一化载频
		switch (FcOut)
		{
		case min:
		{
			fcOut = std::min(fc1, fc2);
			break;
		}
		case center:
		{
			fcOut = (fc1 + fc2) / 2.0;
			break;
		}
		case max:
		{
			fcOut = std::max(fc1, fc2);
			break;
		}
		}
		output[i].SetCharacterizationFrequency(fcOut);
	}

	return bStatus;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool RADAR_SummerBusRF::Run()
{
	bool bStatus = true;

	double dTime = output[0].GetTime(0, GetCount());

	int	ChannelNumIn1 = input1.GetSize();
	int	ChannelNumIn2 = input2.GetSize();
	int	ChannelNumOut = output.GetSize();

	// 两个输入以及输出端口的通道数需要一致
	if (ChannelNumIn1 != ChannelNumIn2)
	{
		POST_ERROR("The width of input1 and input2 should be the same.");
		bStatus = false;
	}

	if (ChannelNumIn1 != ChannelNumOut)
	{
		POST_ERROR("The width of input and output should be the same.");
		bStatus = false;
	}

	for (int i = 0; i < ChannelNumOut; i++)
	{
		fc1 = input1[i].GetCharacterizationFrequency();
		fc2 = input2[i].GetCharacterizationFrequency();
		fcOut = output[i].GetCharacterizationFrequency();
		output[i][0] = input1[i][0].ConvertToNewFc(fc1, fcOut, dTime) + input2[i][0].ConvertToNewFc(fc2, fcOut, dTime);
	}

	return bStatus;
}
