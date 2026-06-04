#include "RADAR_JammerLocation.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( RADAR_JammerLocation )
{	
	SET_MODEL_DESCRIPTION("Measure the range of jammer");

	SET_MODEL_CATEGORY("EW");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(input);
		port.SetDescription("The input of range-doppler matrix");
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(Range);
		port.SetDescription("The measured range which maybe is ambiguous.");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(PRI);
		param.SetDescription("Pulse Repetition Interval");
		param.SetUnit(SystemVueModelBuilder::Units::TIME);
		param.SetDefaultValue("1e-4");
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

RADAR_JammerLocation::RADAR_JammerLocation()
{

}

bool RADAR_JammerLocation::Setup()
{
	bool bStatus = true;

	if (PRI * SampleRate > 0)
	{
		input.SetRate(PRI * SampleRate);
	}
	else
	{
		POST_ERROR("input port rate PRI * SampleRate must be greater than 0.");
		bStatus = false;
	}
	return bStatus;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool RADAR_JammerLocation::Run()
{
	// 快时间维求距离量
	double maxValue = 0.0;
	int maxIndex = 0;
	for (int i = 0; i < PRI * SampleRate; i++)
	{
		if (input[i] > maxValue)
		{
			maxValue = input[i];
			maxIndex = i;
		}
	}

	maxIndex = fmod(maxIndex, PRI * SampleRate);

	const double c = 3e8;
	Range[0] = c * (maxIndex / SampleRate) / 2;
	return true;
}
