#include "RADAR_RangeMeas.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( RADAR_RangeMeas )
{	
	SET_MODEL_DESCRIPTION("Measure the range");

	SET_MODEL_CATEGORY("Measurement");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(input);
		port.SetDescription("The input of range-doppler matrix");
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(Range);
		port.SetDescription("The measured range which maybe is ambiguous.");
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(Index);
		port.SetDescription("The index value of maximum value in the range-doppler matrix.");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(PRI);
		param.SetDescription("Pulse Repetition Interval");
		param.SetUnit(SystemVueModelBuilder::Units::TIME);
		param.SetDefaultValue("1e-4");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(CPI_Num);
		param.SetDescription("The number of pulses in one CPI");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("32");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(SampleRate);
		param.SetDescription("Waveform Baseband Sampling Rate");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("10e6");
	}
	return true;
}
#endif

RADAR_RangeMeas::RADAR_RangeMeas()
{

}

bool RADAR_RangeMeas::Setup()
{
	bool bStatus = true;
	PRINum = PRI * SampleRate;
	portRate = PRINum * CPI_Num;
	if (portRate > 0)
	{
		input.SetRate(portRate);
	}
	else
	{
		POST_ERROR("input port rate PRI * SampleRate * CPI_Num must be greater than 0.");
		bStatus = false;
	}
	return bStatus;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool RADAR_RangeMeas::Run()
{
	// 慢时间维求距离量
	double maxValue = 0.0;
	int maxIndex = 0;
	for (int i = 0; i < portRate; i++)
	{
		if (input[i] > maxValue)
		{
			maxValue = input[i];
			maxIndex = i;
		}
	}

	maxIndex = fmod(maxIndex, PRINum);

	const double c = 3e8;
	Range[0] = c * (maxIndex / SampleRate) / 2;
	Index[0] = maxIndex;
	return true;
}
