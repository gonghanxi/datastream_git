#include "PeakDetector.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( PeakDetector )
{	
	SET_MODEL_DESCRIPTION("Peak Detector");
	SET_MODEL_SYMBOL("SYM_PeakDetector");
	SET_MODEL_CATEGORY("Analog/RF");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(input);
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(ChargeTimeConstant);
		param.SetDescription("Output voltage charge time constant");
		param.SetUnit(SystemVueModelBuilder::Units::TIME);
		param.SetDefaultValue("0");
		param.SetSchematicDisplay(0);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(DecayTimeConstant);
		param.SetDescription("Output voltage decay time constant");
		param.SetUnit(SystemVueModelBuilder::Units::TIME);
		param.SetDefaultValue("20e-6");
		param.SetSchematicDisplay(0);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(VThreshold);
		param.SetDescription("Voltage threshold for detection");
		param.SetUnit(SystemVueModelBuilder::Units::VOLTAGE);
		param.SetDefaultValue("0");
		param.SetSchematicDisplay(0);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(VTransWidth);
		param.SetDescription("Voltage transition width");
		param.SetUnit(SystemVueModelBuilder::Units::VOLTAGE);
		param.SetDefaultValue("0");
		param.SetSchematicDisplay(0);
	}

	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(Polarity, SelectedPolarity);
		enumParam.SetDescription("Polarity of the peak detector: positive, negative");
		enumParam.AddEnumeration("positive", positive);
		enumParam.AddEnumeration("negative", negative);
		enumParam.SetDefaultValue("0");
		enumParam.SetSchematicDisplay(0);
	}
	return true;
}
#endif

PeakDetector::PeakDetector()
{

}

bool PeakDetector::Setup()
{
	bool bStatus = true;

	if (ChargeTimeConstant < 0)
	{
		POST_ERROR("ChargeTimeConstant must be >= 0.");
		bStatus = false;
	}

	if (DecayTimeConstant < 0)
	{
		POST_ERROR("DecayTimeConstant must be >= 0.");
		bStatus = false;
	}

	if (VTransWidth < 0)
	{
		POST_ERROR("VTransWidth must be >= 0.");
		bStatus = false;
	}

	VOut = 0;

	return bStatus;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool PeakDetector::Run()
{
	SampleRate = input.GetSampleRate();
	polaritySign = Polarity ? -1 : 1;
	VSignal = input[0].imag() ? polaritySign * std::abs(input[0].complex()) : input[0].real();

	// 输入信号检测处理阶段
	switch (Polarity)
	{
	case PeakDetector::positive:
		if (VThreshold < 0)
		{
			POST_ERROR("Whren Polarity = positive, VThreshold must be >= 0.");
			return false;
		}

		if (VTransWidth == 0)
		{
			if (VSignal >= VThreshold)
			{
				VDetect = VSignal - VThreshold;
			}
			else
			{
				VDetect = 0;
			}
		}
		else
		{
			if (VSignal < VThreshold - VTransWidth / 2)
			{
				VDetect = 0;
			}
			else if (VSignal > VThreshold + VTransWidth / 2)
			{
				VDetect = VSignal - VThreshold;
			}
			else
			{
				VDetect = 0.5 / VTransWidth * std::pow((VSignal - (VThreshold - VTransWidth / 2)), 2);
			}
		}
		break;

	case PeakDetector::negative:
		if (VThreshold > 0)
		{
			POST_ERROR("Whren Polarity = negative, VThreshold must be <= 0.");
			return false;
		}

		if (VTransWidth == 0)
		{
			if (VSignal < VThreshold)
			{
				VDetect = VSignal - VThreshold;
			}
			else
			{
				VDetect = 0;
			}
		}
		else
		{
			if (VSignal > VThreshold + VTransWidth / 2)
			{
				VDetect = 0;
			}
			else if (VSignal < VThreshold - VTransWidth / 2)
			{
				VDetect = VSignal - VThreshold;
			}
			else
			{
				VDetect = 0.5 / VTransWidth * std::pow((VSignal - (VThreshold + VTransWidth / 2)), 2);
			}
		}
		break;

	default:
		break;
	}

	// 信号输出处理阶段
	if (ChargeTimeConstant == 0 && VDetect > VOut)
	{
		VOut = VDetect;
	}

	else if (ChargeTimeConstant > 0 && VDetect > VOut)
	{
		VTest = VOut + (VDetect - VOut)*(1.0 - std::exp(-1 / ChargeTimeConstant / SampleRate));
		if (VTest > VOut)
		{
			VOut = VTest;
		}
	}

	if (DecayTimeConstant == 0 && VDetect < VOut)
	{
		VOut = VDetect;
	}

	else if (DecayTimeConstant > 0 && VDetect < VOut)
	{
		VTest = VOut * std::exp(-1 / DecayTimeConstant / SampleRate);
		if (VTest < VOut)
		{
			VOut = VTest;
		}
	}

	output[0] = VOut;
	return true;
}
