#include "RealToInt.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( RealToInt )
{	
	SET_MODEL_DESCRIPTION("Convert float to int");
	SET_MODEL_CATEGORY("Type Converters");
	ADD_MODEL_INPUT( input );
	ADD_MODEL_OUTPUT( output );
	
	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(ConvertType, SelectedConvertType);
		enumParam.AddEnumeration("Static_Cast", Static_Cast);
		enumParam.AddEnumeration("Floor", Floor);
		enumParam.AddEnumeration("Ceil", Ceil);
		enumParam.AddEnumeration("Round", Round);
		enumParam.SetDefaultValue("0");
		enumParam.SetSchematicDisplay(0);
	}
	return true;
}
#endif

RealToInt::RealToInt()
{

}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool RealToInt::Run()
{
	switch (ConvertType)
	{
	case RealToInt::Static_Cast:
		output[0] = static_cast<int>(input[0]);
		break;
	case RealToInt::Floor:
		output[0] = std::floor(input[0]);
		break;
	case RealToInt::Ceil:
		output[0] = std::ceil(input[0]);
		break;
	case RealToInt::Round:
		output[0] = std::round(input[0]);
		break;
	default:
		break;
	}
	return true;
}
