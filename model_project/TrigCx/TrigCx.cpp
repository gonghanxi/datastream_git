#include "TrigCx.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( TrigCx )
{	
	SET_MODEL_DESCRIPTION("Complex Trigonometric Function");
	SET_MODEL_SYMBOL("SYM_Trig");
	SET_MODEL_CATEGORY("Math Scalar");

	ADD_MODEL_INPUT( input );
	ADD_MODEL_OUTPUT( output );
	
	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(FunctionType, SelectedFunctionType);
		enumParam.SetDescription("Trigonometric function type: Sin, Cos, Tan, Cot, Asin, Acos, Atan, Acot, Sinh, Cosh, Tanh, Coth, Asinh, Acosh, Atanh, Acoth");
		enumParam.AddEnumeration("Sin", Sin);
		enumParam.AddEnumeration("Cos", Cos);
		enumParam.AddEnumeration("Tan", Tan);
		enumParam.AddEnumeration("Cot", Cot);
		enumParam.AddEnumeration("Asin", Asin);
		enumParam.AddEnumeration("Acos", Acos);
		enumParam.AddEnumeration("Atan", Atan);
		enumParam.AddEnumeration("Acot", Acot);
		enumParam.AddEnumeration("Sinh", Sinh);
		enumParam.AddEnumeration("Cosh", Cosh);
		enumParam.AddEnumeration("Tanh", Tanh);
		enumParam.AddEnumeration("Coth", Coth);
		enumParam.AddEnumeration("Asinh", Asinh);
		enumParam.AddEnumeration("Acosh", Acosh);
		enumParam.AddEnumeration("Atanh", Atanh);
		enumParam.AddEnumeration("Acoth", Acoth);
		enumParam.SetDefaultValue("0");
	}
	return true;
}
#endif

TrigCx::TrigCx()
{
	
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool TrigCx::Run()
{
	const double PI = std::acos(-1);

	switch (FunctionType)
	{
	case TrigCx::Sin:
		output[0] = std::sin(input[0]);
		break;
	case TrigCx::Cos:
		output[0] = std::cos(input[0]);
		break;
	case TrigCx::Tan:
		output[0] = std::tan(input[0]);
		break;
	case TrigCx::Cot:
		if (input[0] == 0.0)
		{
			POST_ERROR("Cot input is out of domain.(z|z』0)");
		}
		else
		{
			output[0] = 1.0 / std::tan(input[0]);
		}
		break;
	case TrigCx::Asin:
		output[0] = std::asin(input[0]);
		break;
	case TrigCx::Acos:
		output[0] = std::acos(input[0]);
		break;
	case TrigCx::Atan:
		output[0] = std::atan(input[0]);
		break;
	case TrigCx::Acot:
		if (input[0] == 0.0)
		{
			POST_ERROR("Acot input is out of domain.(z|z』0)");
		}
		else
		{
			output[0] = std::atan(1.0 / input[0]);
		}
		break;
	case TrigCx::Sinh:
		output[0] = std::sinh(input[0]);
		break;
	case TrigCx::Cosh:
		output[0] = std::cosh(input[0]);
		break;
	case TrigCx::Tanh:
		output[0] = std::tanh(input[0]);
		break;
	case TrigCx::Coth:
		if (input[0] == 0.0)
		{
			POST_ERROR("Coth input is out of domain.(z|z』0)");
		}
		else
		{
			output[0] = 1.0 / std::tanh(input[0]);
		}
		break;
	case TrigCx::Asinh:
		output[0] = std::asinh(input[0]);
		break;
	case TrigCx::Acosh:
		output[0] = std::acosh(input[0]);
		break;
	case TrigCx::Atanh:
		output[0] = std::atanh(input[0]);
		break;
	case TrigCx::Acoth:
		if (input[0] == 0.0)
		{
			POST_ERROR("Acoth input is out of domain.(z|z』0)");
		}
		else
		{
			output[0] = std::atanh(1.0 / input[0]);
		}
		break;
	default:
		break;
	}
	return true;
}
