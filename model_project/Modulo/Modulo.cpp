#include "Modulo.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( Modulo )
{	
	SET_MODEL_DESCRIPTION("Modulo");
	SET_MODEL_SYMBOL("SYM_Modulo");
	SET_MODEL_CATEGORY("Math Scalar");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(input);
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(moduloValue);
		param.SetName("Modulo");
		param.SetDescription("Modulo value");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("1");
	}
	return true;
}
#endif

Modulo::Modulo()
{

}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool Modulo::Run()
{
	output[0] = std::fmod(input[0], moduloValue);
	return true;
}
