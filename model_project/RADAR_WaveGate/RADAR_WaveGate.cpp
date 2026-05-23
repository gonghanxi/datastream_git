#include "RADAR_WaveGate.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( RADAR_WaveGate )
{	
	SET_MODEL_DESCRIPTION("Wave Gate to capture and process the echo signal");

	SET_MODEL_CATEGORY("Signal Processing");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(input);
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(GateStartCtrl);
		port.SetOptional();
	}
	
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(PRF);
		param.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		param.SetDefaultValue("10e3");
	}
	
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(StartTime);
		param.SetUnit(SystemVueModelBuilder::Units::TIME);
		param.SetDefaultValue("0");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(GateTime);
		param.SetUnit(SystemVueModelBuilder::Units::TIME);
		param.SetDefaultValue("20e-6");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(SampleRate);
		param.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		param.SetDefaultValue("10e6");
	}
	return true;
}
#endif

RADAR_WaveGate::RADAR_WaveGate()
{

}

//-----------------------------------------------------------------------------------
//	Setup
//		Port rate should be set here
//-----------------------------------------------------------------------------------
bool RADAR_WaveGate::Setup()
{
	bool bStatus = true;

	double PRI = 1.0 / PRF;

	int inputRate = PRI * SampleRate;
	int outputRate = GateTime * SampleRate;

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
bool RADAR_WaveGate::Run()
{
	bool bStatus = true;

	if (GateStartCtrl.IsConnected())
	{
		StartTime = GateStartCtrl[0];
	}

	int StartN	= StartTime * SampleRate;
	int GateN = GateTime * SampleRate;
	int StopN = StartN + GateN;

	// 每个PRI都设一个波门
	for (int i = 0; i < StopN; i++)
	{
		output[i] = input[i + StartN];
	}

	return bStatus;
}
