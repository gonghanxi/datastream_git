#include "MaxMin.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( MaxMin )
{	
	SET_MODEL_DESCRIPTION("Maximum or Minimum Value Function");
	SET_MODEL_SYMBOL("SYM_MaxMin");
	SET_MODEL_CATEGORY("Signal Processing");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(input);
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(index);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(N);
		param.SetDescription("Number of samples");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("10");
	}

	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(MaxOrMin, SelectedMaxOrMin);
		enumParam.SetDescription("Output value: min, max");
		enumParam.AddEnumeration("min", min);
		enumParam.AddEnumeration("max", max);
		enumParam.SetDefaultValue("1");
	}

	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(Compare, SelectedCompare);
		enumParam.SetDescription("Compare input value or magnitude: valueIn, magnitudeIn");
		enumParam.AddEnumeration("valueIn", valueIn);
		enumParam.AddEnumeration("magnitudeIn", magnitudeIn);
		enumParam.SetDefaultValue("0");
		enumParam.SetSchematicDisplay(0);
		enumParam.SetUseDefault(1);
	}

	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(OutputType, SelectedOutputType);
		enumParam.SetDescription("Compare input value or magnitude: valueOut, magnitudeOut");
		enumParam.AddEnumeration("valueOut", valueOut);
		enumParam.AddEnumeration("magnitudeOut", magnitudeOut);
		enumParam.SetDefaultValue("0");
		enumParam.SetSchematicDisplay(0);
		enumParam.SetUseDefault(1);
	}
	return true;
}
#endif

MaxMin::MaxMin()
{

}

bool MaxMin::Setup()
{
	bool bStatus = true;
	if (N >= 1)
	{
		input.SetRate(N);
	}
	else
	{
		POST_ERROR("Number of samples to compare must be greater than 1.");
		bStatus = false;
	}
	return bStatus;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool MaxMin::Run()
{

	if (MaxOrMin == MaxMin::min)
	{
		// 使用输入第一个值进行初始化
		int minIndex = 0;
		double minValue = input[0];
		double minMagnitude = std::abs(input[0]);

		for (int i = 1; i < N; i++)
		{
			if (Compare == MaxMin::valueIn && input[i] < minValue)
			{
				minIndex = i;
				minValue = input[i];
			}

			if (Compare == MaxMin::magnitudeIn && std::abs(input[i]) < minMagnitude)
			{
				minIndex = i;
				minMagnitude = abs(input[i]);
			}
		}

		index[0] = minIndex;
		if (OutputType == MaxMin::valueOut)
		{
			output[0] = input[minIndex];
		}
		if (OutputType == MaxMin::magnitudeOut)
		{
			output[0] = std::abs(input[minIndex]);
		}
	}

	if (MaxOrMin == MaxMin::max)
	{
		// 使用输入第一个值进行初始化
		int maxIndex = 0;
		double maxValue = input[0];
		double maxMagnitude = std::abs(input[0]);

		for (int i = 1; i < N; i++)
		{
			if (Compare == MaxMin::valueIn && input[i] > maxValue)
			{
				maxIndex = i;
				maxValue = input[i];
			}

			if (Compare == MaxMin::magnitudeIn && std::abs(input[i]) > maxMagnitude)
			{
				maxIndex = i;
				maxMagnitude = abs(input[i]);
			}
		}

		index[0] = maxIndex;
		if (OutputType == MaxMin::valueOut)
		{
			output[0] = input[maxIndex];
		}
		if (OutputType == MaxMin::magnitudeOut)
		{
			output[0] = std::abs(input[maxIndex]);
		}
	}
	return true;
}
