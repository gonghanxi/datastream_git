#include "Unwrap.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( Unwrap )
{	
	SET_MODEL_DESCRIPTION("Unwrap function");
	SET_MODEL_SYMBOL("SYM_Unwrap");
	SET_MODEL_CATEGORY("Math Scalar");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(input);
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
	}

	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(PhaseType, SelectedPhaseType);
		enumParam.SetDescription("Phase type for the input and output signals: radians, degrees");
		enumParam.AddEnumeration("radians", radians);
		enumParam.AddEnumeration("degrees", degrees);
		enumParam.SetDefaultValue("0");
		enumParam.SetSchematicDisplay(0);
		enumParam.SetUseDefault(1);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(OutPhase);
		param.SetDescription("Initial output phase");
		param.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		param.SetDefaultValue("0");
		param.SetSchematicDisplay(0);
		param.SetUseDefault(1);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(PrevPhase);
		param.SetDescription("Initial wrapped phase of input signal for computing the first phase difference");
		param.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		param.SetDefaultValue("0");
		param.SetSchematicDisplay(0);
		param.SetUseDefault(1);
	}
	return true;
}
#endif

Unwrap::Unwrap()
{
	
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool Unwrap::Run()
{
	const double PI = std::acos(-1);
	double Period = (PhaseType == Unwrap::radians) ? 2.0 * PI : 360.0;
	double PhaseDifference = std::fmod(input[0] - PrevPhase, Period);
	OutPhase += PhaseDifference;
	output[0] = OutPhase;
	PrevPhase = input[0];
	return true;
}
