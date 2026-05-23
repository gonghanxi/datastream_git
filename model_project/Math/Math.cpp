#include "MathModel.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( Math )
{	
	SET_MODEL_DESCRIPTION("Math Function");
	SET_MODEL_SYMBOL("SYM_Math");
	SET_MODEL_CATEGORY("Math Scalar");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(input);
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
	}

	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(FunctionType, SelectedFunctionType);
		enumParam.SetDescription("Mathematical function: Abs, Ceil, Exp, Floor, Ln, Log10, Pow10, Recip, Round, Sqr, Sqrt, Sgn");
		enumParam.AddEnumeration("Abs", Abs);		// 0
		enumParam.AddEnumeration("Ceil", Ceil);		// 1
		enumParam.AddEnumeration("Exp", Exp);		// 2
		enumParam.AddEnumeration("Floor", Floor);	// 3
		enumParam.AddEnumeration("Ln", Ln);			// 4
		enumParam.AddEnumeration("Log10", Log10);	// 5
		enumParam.AddEnumeration("Pow10", Pow10);	// 6
		enumParam.AddEnumeration("Recip", Recip);	// 7
		enumParam.AddEnumeration("Round", Round);	// 8
		enumParam.AddEnumeration("Sqr", Sqr);		// 9
		enumParam.AddEnumeration("Sqrt", Sqrt);		// 10
		enumParam.AddEnumeration("Sgn", Sgn);		// 11
		enumParam.SetDefaultValue("0");
	}
	return true;
}
#endif

Math::Math()
{

}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool Math::Run()
{
	switch (FunctionType)
	{
	case Math::Abs:
		output[0] = abs(input[0]);
		break;

	case Math::Ceil:
		output[0] = ceil(input[0]);
		break;

	case Math::Exp:
		output[0] = exp(input[0]);
		break;

	case Math::Floor:
		output[0] = floor(input[0]);
		break;

	case Math::Ln:
		output[0] = log(input[0]);
		break;

	case Math::Log10:
		output[0] = log10(input[0]);
		break;

	case Math::Pow10:
		output[0] = pow(10, input[0]);
		break;

	case Math::Recip:
		output[0] = 1 / input[0];
		break;

	case Math::Round:
		output[0] = round(input[0]);
		break;

	case Math::Sqr:
		output[0] = input[0] * input[0];
		break;

	case Math::Sqrt:
		output[0] = sqrt(input[0]);
		break;

	case Math::Sgn:
		if (input[0] > 0)
		{
			output[0] = 1;
		}
		if (input[0] == 0)
		{
			output[0] = 0;
		}
		if (input[0] < 0)
		{
			output[0] = -1;
		}
		break;

	default:
		break;
	}
	return true;
}
