#include "RADAR_Switch.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( RADAR_Switch )
{	
	SET_MODEL_DESCRIPTION("Tx/Rx switch");

	SET_MODEL_CATEGORY("Tx/Rx");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(input);
		port.SetDescription("The input signal");
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(PRI);
		port.SetDescription("PRI control signal");
		port.SetOptional();
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
		port.SetDescription("The output signal after switch");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(PRF);
		param.SetDescription("Pulse Repetition Frequency");
		param.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		param.SetDefaultValue("10e3");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(SwitchOff_Time);
		param.SetDescription("The time to block signal into receiver when transmitter works");
		param.SetUnit(SystemVueModelBuilder::Units::TIME);
		param.SetDefaultValue("5e-6");
	}
	return true;
}
#endif

RADAR_Switch::RADAR_Switch()
{

}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool RADAR_Switch::Run()
{
	double t = output.GetStartTime() + static_cast<double>(GetCount()) * output.GetTimeStep() + 1e-16;
	double pri = 1.0 / PRF;

	if (PRI.IsConnected())
	{
		pri = PRI[0];
	}

	if (fmod(t, pri) < SwitchOff_Time)
	{
		output[0] = 0.0;
	}

	else
	{
		output[0] = input[0];
	}
	return true;
}

