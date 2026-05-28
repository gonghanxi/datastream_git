#include "SwitchSPST.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( SwitchSPST )
{	
	SET_MODEL_DESCRIPTION("Single Pole Single Throw Switch");
	SET_MODEL_SYMBOL("SYM_SwitchSPST");
	SET_MODEL_CATEGORY("Analog/RF");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(input);
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(control);
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(Loss);
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("0");
		param.SetDescription("Loss in dB for on state insertion loss");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(Isolation);
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("200");
		param.SetDescription("Isolation in dB for off state insertion loss");
		param.SetSchematicDisplay(0);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(VThreshold);
		param.SetUnit(SystemVueModelBuilder::Units::VOLTAGE);
		param.SetDefaultValue("0.5");
		param.SetDescription("Control voltage threshold");
		param.SetSchematicDisplay(0);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(TOn);
		param.SetUnit(SystemVueModelBuilder::Units::TIME);
		param.SetDefaultValue("0");
		param.SetDescription("On-state transition ctime for output");
		param.SetSchematicDisplay(0);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(TOff);
		param.SetUnit(SystemVueModelBuilder::Units::TIME);
		param.SetDefaultValue("0");
		param.SetDescription("Off-state transition ctime for output");
		param.SetSchematicDisplay(0);
	}
	return true;
}
#endif

SwitchSPST::SwitchSPST()
{

}

bool SwitchSPST::Setup()
{
	bool bStatus = true;

	if (VThreshold <= 0)
	{
        LOG_ERROR("VThreshold must be > 0");
		bStatus = false;
	}
	if (TOn < 0)
	{
        LOG_ERROR("TOn must be >= 0");
		bStatus = false;
	}
	if (TOff < 0)
	{
        LOG_ERROR("TOff must be >= 0");
		bStatus = false;
	}

	SwitchState = false;
	Ts = 0;

	return bStatus;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool SwitchSPST::Run()
{
	double t = input.GetTime(0, m_iFiringCount);

	if (control[0].real() > VThreshold)
	{
		if (!SwitchState)
		{
			SwitchState = true;
			Ts = t;
		}

		if (t >= Ts + TOn)
		{
			output[0] = std::pow(10, -(Loss / 20))*input[0].complex();
		}
		else
		{
			output[0] = (std::pow(10, -(Loss / 20)) - std::pow(10, -(Isolation / 20)))*(t - Ts) / TOn * input[0].complex() + std::pow(10, -(Isolation / 20))*input[0].complex();
		}
	}
	else if (control[0].real() <= VThreshold)
	{
		if (SwitchState)
		{
			SwitchState = false;
			Ts = t;
		}

		if (t >= Ts + TOff)
		{
			output[0] = std::pow(10, -(Isolation / 20))*input[0].complex();
		}
		else
		{
			output[0] = (std::pow(10, -(Loss / 20)) - std::pow(10, -(Isolation / 20)))*(1 - (t - Ts) / TOff) * input[0].complex() + std::pow(10, -(Isolation / 20))*input[0].complex();
		}
	}
	return true;
}
