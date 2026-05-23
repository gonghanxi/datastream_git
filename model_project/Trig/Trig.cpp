#include "Trig.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( Trig )
{	
	SET_MODEL_DESCRIPTION("Trigonometric Function");
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

Trig::Trig()
{
	
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool Trig::Run()
{
	bool bStatus = true;
	const double PI = acos(-1);
	switch (FunctionType)
	{
	case Trig::Sin:
		output[0] = std::sin(input[0]);
		break;
	case Trig::Cos:
		output[0] = std::cos(input[0]);
		break;
	case Trig::Tan:
		if (std::cos(input[0])==0)
		{
			POST_ERROR("Tan input is out of domain.(x|x¡Ùk¦Ð+¦Ð/2, k¡ÊZ)");
			bStatus = false;
		}
		else 
		{
			output[0] = std::tan(input[0]);
		}
		break;
	case Trig::Cot:
		if (std::sin(input[0])==0)
		{
			POST_ERROR("Cot input is out of domain.(x|x¡Ùk¦Ð, k¡ÊZ)");
			bStatus = false;
		}
		else
		{
			output[0] = 1 / std::tan(input[0]);
		}
		break;
	case Trig::Asin:
		if (input[0] < -1 || input[0] > 1)
		{
			POST_ERROR("Asin input is out of domain.(x|x¡Ê[-1,1])");
			bStatus = false;
		}
		else
		{
			output[0] = std::asin(input[0]);
		}
		break;
	case Trig::Acos:
		if (input[0] < -1 || input[0] > 1)
		{
			POST_ERROR("Acos input is out of domain.(x|x¡Ê[-1,1])");
			bStatus = false;
		}
		else
		{
			output[0] = std::acos(input[0]);
		}	
		break;
	case Trig::Atan:
		output[0] = std::atan(input[0]);
		break;
	case Trig::Acot:
		output[0] = 0.5*PI - std::atan(input[0]);
		break;
	case Trig::Sinh:
		output[0] = std::sinh(input[0]);
		break;
	case Trig::Cosh:
		output[0] = std::cosh(input[0]);
		break;
	case Trig::Tanh:
		output[0] = std::tanh(input[0]);
		break;
	case Trig::Coth:
		if (input[0] == 0)
		{
			POST_ERROR("Coth input is out of domain. (x|x¡Ù0)");
			bStatus = false;
		}
		else
		{
			output[0] = 1.0 / std::tanh(input[0]);
		}
		break;
	case Trig::Asinh:
		output[0] = std::asinh(input[0]);
		break;
	case Trig::Acosh:
		if (input[0] < 1)
		{
			POST_ERROR("Acosh input is out of domain. (x|x¡Ê[1,+¡Þ))");
			bStatus = false;
		}
		else
		{
			output[0] = std::acosh(input[0]);
		}
		break;
	case Trig::Atanh:
		if (input[0] <= -1 || input[0] >= 1)
		{
			POST_ERROR("Atanh input is out of domain. (x|x¡Ê(-1,1))");
			bStatus = false;
		}
		else
		{
			output[0] = std::atanh(input[0]);
		}
		break;
	case Trig::Acoth:
		if (input[0] >= -1 && input[0] <= 1)
		{
			POST_ERROR("Acoth input is out of domain. (x|x¡Ê(-¡Þ,-1)¡È(1,+¡Þ))");
			bStatus = false;
		}
		else
		{
			output[0] = std::atanh(1.0 / input[0]);
		}
		break;
	default:
		break;
	}
	return bStatus;
}
