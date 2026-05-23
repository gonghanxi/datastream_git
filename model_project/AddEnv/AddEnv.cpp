#include "AddEnv.h"
#include <numeric>

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( AddEnv )
{	
	SET_MODEL_DESCRIPTION("Envelope Signal Adder");
	SET_MODEL_SYMBOL("SYM_AddEnv");
	SET_MODEL_CATEGORY("Analog/RF");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(input);
		port.SetDescription("input signal");
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
		port.SetDescription("output signal");
	}

	{
        SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(OutputFc, SelectedOutputFc);
		enumParam.SetDescription("Output characterization frequency for the combined signal: Min, Max, Center, User defined");
		enumParam.AddEnumeration("min", min);
		enumParam.AddEnumeration("max", max);
		enumParam.AddEnumeration("center", center);
		enumParam.AddEnumeration("User defined", userDefined);
		enumParam.SetDefaultValue("2");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(UserDefinedFc);
		param.SetDescription("User defined output characterization frequency");
		param.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		param.SetDefaultValue("100e6");
        param.SetHideCondition("OutputFc ~= 3");
	}
	return true;
}
#endif

AddEnv::AddEnv()
{

}

//-----------------------------------------------------------------------------------
//	Characterization frequency propagate
//		Unify the characterization frequency.
//-----------------------------------------------------------------------------------
ERESULT AddEnv::PropagateCharacterizationFrequency()
{
	bool bStatus = true;

	fcmax = 0.0;
	fcmean = 0.0;
	fcmin = input[0].GetCharacterizationFrequency(); //取其中一个通道的特征频率对最小值进行初始化

	// 求出各通道输入的最大特征频率、最小特征频率以及平均特征频率

	int	ChannelNumIn = input.GetSize();
	for (int i = 0; i < ChannelNumIn; i++)
	{
		fc = input[i].GetCharacterizationFrequency();

		fcmax = (fcmax < fc ? fc : fcmax);

		fcmin = (fcmin > fc ? fc : fcmin);

		fcmean += fc; // 每个循环累计一个通道内的载频
	}
	fcmean /= ChannelNumIn; // 求平均载频

	// 统一化载频

    switch (OutputFc)
	{
		case min:
		{
			fcOut = fcmin;
			break;
		}
		case max:
		{
			fcOut = fcmax;
			break;
		}
		case center:
		{
			fcOut = fcmean;
			break;
		}
		case userDefined:
		{
			fcOut = UserDefinedFc;
			break;
		}
	}

	output.SetCharacterizationFrequency(fcOut);

	return bStatus;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool AddEnv::Run()
{
	double dTime = output.GetTime(0, GetCount());

	output[0] = 0; // 每次输出重置

	int	ChannelNumIn = input.GetSize();
	for (int i = 0; i < ChannelNumIn; i++)
	{
		fc = input[i].GetCharacterizationFrequency();
		output[0] += input[i][0].ConvertToNewFc(fc, fcOut, dTime);
	}

	return true;
}
