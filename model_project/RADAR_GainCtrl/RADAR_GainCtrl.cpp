#include "RADAR_GainCtrl.h"
#include <cmath>

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( RADAR_GainCtrl )
{	
	SET_MODEL_DESCRIPTION("Gain Control");

	SET_MODEL_CATEGORY("Tx/Rx");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(input);
		port.SetDescription("The input signal");
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(gain);
		port.SetDescription("The gain control input");
		port.SetOptional();
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
		port.SetDescription("The output signal after gain control");
	}

	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(ControlType, SelectedControlType);
		enumParam.SetDescription("The type of gain control: Manual Gain Ctrl, Sensitivity Time Ctrl, Automatic Gain Ctrl, Instantaneous Automatic Gain Ctrl/Fast time constant(FTC)");
		enumParam.AddEnumeration("Manual Gain Ctrl", MGC);
		enumParam.AddEnumeration("Sensitivity Time Ctrl", STC);
		enumParam.AddEnumeration("Instantaneous Automatic Gain Ctrl/Fast time constant(FTC)", AGC);
		enumParam.SetDefaultValue("0");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(PRI);
		param.SetDescription("The PRI value in seconds");
		param.SetUnit(SystemVueModelBuilder::Units::TIME);
		param.SetDefaultValue("10e-3");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(Gain);
		param.SetDescription("The power gain in dB");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("0");
		param.SetHideCondition("ControlType ~= 0");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(STC_Factor);
		param.SetDescription("The power gain control factor, the general value is 4. The factor is set to 3 for area clutter and is set to 2 for volume clutter and chaff");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("4");
		param.SetHideCondition("ControlType ~= 1");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(STC_StartTime);
		param.SetDescription("The STC start time in each PRI");
		param.SetUnit(SystemVueModelBuilder::Units::TIME);
		param.SetDefaultValue("2e-6");
		param.SetHideCondition("ControlType ~= 1");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(STC_StopTime);
		param.SetDescription("The STC stop time in each PRI");
		param.SetUnit(SystemVueModelBuilder::Units::TIME);
		param.SetDefaultValue("60e-6");
		param.SetHideCondition("ControlType ~= 1");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(STC_K_Coef);
		param.SetDescription("The coefficient value in the equation Pc = K_Coef * R^(-STC_Factor)");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("1e-4");
		param.SetHideCondition("ControlType ~= 1");
	}

	return true;
}
#endif

RADAR_GainCtrl::RADAR_GainCtrl()
{

}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool RADAR_GainCtrl::Run()
{
	double t = output.GetStartTime() + static_cast<double>(GetCount()) * output.GetTimeStep();

	if (gain.IsConnected())
	{
		Gain = gain[0];
	}

	switch (ControlType)
	{
		case MGC:
		{
			output[0] = input[0] * pow(10, Gain / 10);
			break;
		}
		case STC:
		{
			const double c = 3e8;
			//const double c = 299792458;
			double Pc = 1.0;
			double tInPRI = fmod(t, PRI);

			if (tInPRI >= STC_StartTime && tInPRI < STC_StopTime)
			{
				double R = c * tInPRI / 2.0; 
				Pc = STC_K_Coef * pow(R, STC_Factor) * pow(10, -4 * (STC_Factor - 1));
			}
			else if (tInPRI >= STC_StopTime)
			{
				double R = c * STC_StopTime / 2.0;
				Pc = STC_K_Coef * pow(R, STC_Factor) * pow(10, -4 * (STC_Factor - 1));
			}
			output[0] = input[0] * Pc;

			break;
		}

		// SystemVue中实际尚未实现 AGC，不论输入是什么输出均为 0 //
		case AGC:
		{
			output[0] = 0.0; // 占位输出 //
			break;
		}
	}

	return true;
}

