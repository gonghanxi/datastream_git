#include "RADAR_CoIntgr.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( RADAR_CoIntgr )
{	
	SET_MODEL_DESCRIPTION("Signal Coherenent Integration");

	SET_MODEL_CATEGORY("Signal Processing");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(input);
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(PRI_Or_WaveGate);
		param.SetUnit(SystemVueModelBuilder::Units::TIME);
		param.SetDefaultValue("10e-3");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(NumOfPulse);
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("32");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(SampleRate);
		param.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		param.SetDefaultValue("10e6");
	}
	return true;
}
#endif

RADAR_CoIntgr::RADAR_CoIntgr()
{

}

//-----------------------------------------------------------------------------------
//	Setup
//		Port rate should be set here
//-----------------------------------------------------------------------------------
bool RADAR_CoIntgr::Setup()
{
	bool bStatus = true;

	int inputRate = PRI_Or_WaveGate * SampleRate * NumOfPulse;
	int outputRate = PRI_Or_WaveGate * SampleRate;

	if (inputRate > 0 && outputRate > 0)
	{
		input.SetRate(inputRate);
		output.SetRate(outputRate);
	}

	else
	{
		POST_ERROR("Port rate must be greater than 0.");
		bStatus = false;
	}
	return bStatus;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool RADAR_CoIntgr::Run()
{
	int PRN = PRI_Or_WaveGate * SampleRate; // 每个脉冲重复周期或波门内的点数

	for (int i = 0; i < PRN; i++)
	{
		output[i].real(0.0);
		output[i].imag(0.0);

		// 对 NumOfPulse 个脉冲做相参积累
		for (int PulseIndex = 0; PulseIndex < NumOfPulse; PulseIndex++)
		{
			output[i] += input[PulseIndex*PRN + i];
		}
	}
	return true;
}
