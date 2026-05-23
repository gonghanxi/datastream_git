#include "LogVDet.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( LogVDet )
{	
	SET_MODEL_DESCRIPTION(" Logarithmic Video Detector");
	SET_MODEL_SYMBOL("SYM_LogVDet");
	SET_MODEL_CATEGORY("Analog/RF");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(input);
		port.SetDescription("input signal");
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
		port.SetDescription("output signal");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(Sensitivity);
		param.SetDescription("Log sensitivity in volts/dB");
		param.SetUnit(SystemVueModelBuilder::Units::VOLTAGE);
		param.SetDefaultValue("0.1");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(PMin);
		param.SetDescription("Minimum input power in dBm for logarithmic amplification");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("-80");
		param.SetSchematicDisplay(0);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(E);
		param.SetDescription("Peak log error in dB");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("0");
		param.SetSchematicDisplay(0);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(Ec);
		param.SetDescription("Log error cycle in dB");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("0");
		param.SetSchematicDisplay(0);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(RefR);
		param.SetDescription("Reference resistance");
		param.SetUnit(SystemVueModelBuilder::Units::RESISTANCE);
		param.SetDefaultValue("50");
		param.SetSchematicDisplay(0);
	}
	return true;
}
#endif

LogVDet::LogVDet()
{
	
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool LogVDet::Run()
{
	const double PI = std::acos(-1);

	double At = std::abs(input[0].complex());
	double PA = 10 * std::log10(0.5*At*At / RefR) + 30;
	double ep = Sensitivity * E*std::sin(2 * PI*(PA - PMin) / Ec);
	double VL = std::sqrt(2 * RefR*std::pow(10, (PMin - 30) / 10));

	output[0] = At > VL ? 20 * Sensitivity*std::log10(At / VL) + ep : 0;
	return true;
}
