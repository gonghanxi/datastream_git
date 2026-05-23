#include "EnvToCx.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( EnvToCx )
{	
	SET_MODEL_DESCRIPTION("Convert envelope signal to complex signal");
	SET_MODEL_SYMBOL("SYM_EnvToCx");
	SET_MODEL_CATEGORY("Analog/RF");
	SET_MODEL_CATEGORY("Type Converters");

	ADD_MODEL_INPUT(Env);
	ADD_MODEL_OUTPUT(Cx);
	ADD_MODEL_OUTPUT(Fc);
	return true;
}
#endif

EnvToCx::EnvToCx()
{

}

//-----------------------------------------------------------------------------------
//	Characterization frequency propagate
//		Set the characterization frequency from input port Fc (if input).
//-----------------------------------------------------------------------------------

ERESULT EnvToCx::PropagateCharacterizationFrequency()
{
	Fc.SetCharacterizationFrequency(Env.GetCharacterizationFrequency());
	return true;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool EnvToCx::Run()
{
	Cx[0] = Env[0].complex();
	return true;
}
