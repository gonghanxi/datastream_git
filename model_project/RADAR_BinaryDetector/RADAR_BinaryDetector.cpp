#include "RADAR_BinaryDetector.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( RADAR_BinaryDetector )
{	
	SET_MODEL_DESCRIPTION("Binary Detector");

	SET_MODEL_CATEGORY("Signal Processing");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(input);
	}


	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(Threshold);
		param.SetDescription("Threshold is in the [0:1]. When the value of sample is greater than Threshold * maximum value in one detection frame, the output is 1, otherwise, the output is 0.");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("0.6");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(PRI);
		param.SetDescription("Pulse Repetetion Interval");
		param.SetUnit(SystemVueModelBuilder::Units::TIME);
		param.SetDefaultValue("1e-4");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(SampleRate);
		param.SetDescription("Sampling Rate");
		param.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		param.SetDefaultValue("10e6");
	}
	return true;
}
#endif

RADAR_BinaryDetector::RADAR_BinaryDetector()
{
	
}

bool RADAR_BinaryDetector::Setup()
{
	bool bStatus = true;

	// 参数校验
	if (Threshold < 0 || Threshold > 1)
	{
		POST_ERROR("Threshold must be >= 0 and <= 1");
		bStatus = false;
	}

	if (PRI <= 0)
	{
		POST_ERROR("PRI must be > 0");
		bStatus = false;
	}

	if (SampleRate <= 0)
	{
		POST_ERROR("SampleRate must be > 0");
		bStatus = false;
	}

	numPRI = static_cast<int>(PRI * SampleRate);
	input.SetRate(numPRI);
	output.SetRate(numPRI);

	return bStatus;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool RADAR_BinaryDetector::Run()
{
	// 求单个PRI内最大值以获得门限
	double maxValue = 0.0;
	for (int i = 0; i < numPRI; i++)
	{
		if (input[i] > maxValue)
		{
			maxValue = input[i];
		}
	}
	double thresholdValue = maxValue * Threshold;

	// 门限判决输出
	for (int i = 0; i < numPRI; i++)
	{
		output[i] = input[i] > thresholdValue ? 1 : 0;
	}
	
	return true;
}
