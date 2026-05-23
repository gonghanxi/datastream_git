#include "GainInt.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( GainInt )
{	
	SET_MODEL_DESCRIPTION("Constant Gain for integer Signal");
	SET_MODEL_SYMBOL("SYM_Gain");
	SET_MODEL_CATEGORY("Math Matrix");
	SET_MODEL_CATEGORY("Math Scalar");

	ADD_MODEL_INPUT( input );
	ADD_MODEL_OUTPUT( output );

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(m_Gain);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("1");
		p.SetDescription("Gain value");
	}

	return true;
}
#endif

GainInt::GainInt()
{

}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool GainInt::Run()
{
	output[0] = m_Gain * input[0];
	return true;
}
