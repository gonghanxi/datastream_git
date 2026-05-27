#include "IntToReal.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( IntToReal )
{	
	SET_MODEL_DESCRIPTION("Convert int to float");
	SET_MODEL_CATEGORY("Type Converters");
	ADD_MODEL_INPUT( input );
	ADD_MODEL_OUTPUT( output );

	return true;
}
#endif

IntToReal::IntToReal()
{

}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool IntToReal::Run()
{
	output[0] = static_cast<double>(input[0]);
	return true;
}
