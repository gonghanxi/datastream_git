#include "SwitchSPDT.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( SwitchSPDT )
{	
	SET_MODEL_DESCRIPTION("Single Pole Double Throw Switch");
	SET_MODEL_SYMBOL("SYM_SwitchSPDT");
	SET_MODEL_CATEGORY("Analog/RF");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(input);
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(control);
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output1);
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output2);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(Loss1);
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("0");
		param.SetDescription("Loss in dB for on state insertion loss for output 1");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(Isolation1);
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("200");
		param.SetDescription("Isolation in dB for off state insertion loss for output 1");
		param.SetSchematicDisplay(0);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(Loss2);
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("0");
		param.SetDescription("Loss in dB for on state insertion loss for output 2");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(Isolation2);
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("200");
		param.SetDescription("Isolation in dB for off state insertion loss for output 2");
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
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(TOn1);
		param.SetUnit(SystemVueModelBuilder::Units::TIME);
		param.SetDefaultValue("0");
		param.SetDescription("On-state transition ctime for output 1");
		param.SetSchematicDisplay(0);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(TOff1);
		param.SetUnit(SystemVueModelBuilder::Units::TIME);
		param.SetDefaultValue("0");
		param.SetDescription("Off-state transition ctime for output 1");
		param.SetSchematicDisplay(0);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(TOn2);
		param.SetUnit(SystemVueModelBuilder::Units::TIME);
		param.SetDefaultValue("0");
		param.SetDescription("On-state transition ctime for output 2");
		param.SetSchematicDisplay(0);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(TOff2);
		param.SetUnit(SystemVueModelBuilder::Units::TIME);
		param.SetDefaultValue("0");
		param.SetDescription("Off-state transition ctime for output 2");
		param.SetSchematicDisplay(0);
	}
	return true;
}
#endif

SwitchSPDT::SwitchSPDT()
{

}

bool SwitchSPDT::Setup()
{
	bool bStatus = true;

	if (VThreshold <= 0)
	{
		POST_ERROR("VThreshold must be > 0");
		bStatus = false;
	}
	if (TOn1 < 0)
	{
		POST_ERROR("TOn1 must be >= 0");
		bStatus = false;
	}
	if (TOff1 < 0)
	{
		POST_ERROR("TOff1 must be >= 0");
		bStatus = false;
	}
	if (TOn2 < 0)
	{
		POST_ERROR("TOn2 must be >= 0");
		bStatus = false;
	}
	if (TOff2 < 0)
	{
		POST_ERROR("TOff2 must be >= 0");
		bStatus = false;
	}

	return bStatus;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool SwitchSPDT::Run()
{
	double t = input.GetTime(0, m_iFiringCount);

	if (control[0].real() > VThreshold)
	{
		if (!SwitchState)
		{
			SwitchState = true;
			Ts = t;
		}

		if (t >= Ts + TOn1)
		{
			output1[0] = std::pow(10, -(Loss1 / 20))*input[0].complex();
		}
		else if (t < Ts + TOn1)
		{
			output1[0] = (std::pow(10, -(Loss1 / 20)) - std::pow(10, -(Isolation1 / 20)))*(t - Ts) / TOn1 * input[0].complex() + std::pow(10, -(Isolation1 / 20))*input[0].complex();
		}

		if (t >= Ts + TOff2)
		{
			output2[0] = std::pow(10, -(Isolation2 / 20))*input[0].complex();
		}
		else if (t < Ts + TOff2)
		{
			output2[0] = (std::pow(10, -(Loss2 / 20)) - std::pow(10, -(Isolation2 / 20)))*(1 - (t - Ts) / TOff2) * input[0].complex() + std::pow(10, -(Isolation2 / 20))*input[0].complex();
		}
	}
	else if (control[0].real() <= VThreshold)
	{
		if (SwitchState)
		{
			SwitchState = false;
			Ts = t;
		}

		if (t >= Ts + TOn2)
		{
			output2[0] = std::pow(10, -(Loss2 / 20))*input[0].complex();
		}
		else if (t < Ts + TOn2)
		{
			output2[0] = (std::pow(10, -(Loss2 / 20)) - std::pow(10, -(Isolation2 / 20)))*(t - Ts) / TOn2 * input[0].complex() + std::pow(10, -(Isolation2 / 20))*input[0].complex();
		}

		if (t >= Ts + TOff1)
		{
			output1[0] = std::pow(10, -(Isolation1 / 20))*input[0].complex();
		}
		else if (t < Ts + TOff1)
		{
			output1[0] = (std::pow(10, -(Loss1 / 20)) - std::pow(10, -(Isolation1 / 20)))*(1 - (t - Ts) / TOff1) * input[0].complex() + std::pow(10, -(Isolation1 / 20))*input[0].complex();
		}
	}
	return true;
}
