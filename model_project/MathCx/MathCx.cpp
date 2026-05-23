#include "MathCx.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( MathCx )
{	
	SET_MODEL_DESCRIPTION("Complex Math Function");
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
		enumParam.SetDescription("Mathematical function: Abs, Ceil, Exp, Floor, Ln, Log10, Pow10, Recip, Round, Sqr, Sqrt, Conj");
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
		enumParam.AddEnumeration("Conj", Conj);		// 11
		enumParam.SetDefaultValue("0");
	}
	return true;
}
#endif

MathCx::MathCx()
{

}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool MathCx::Run()
{
	switch (FunctionType)
	{
	case MathCx::Abs:
		output[0] = abs(input[0]);
		break;

	case MathCx::Ceil:
		output[0].real(ceil(input[0].real()));
		output[0].imag(ceil(input[0].imag()));
		break;

	case MathCx::Exp:
		output[0] = exp(input[0]);
		break;

	case MathCx::Floor:
		output[0].real(floor(input[0].real()));
		output[0].imag(floor(input[0].imag()));
		break;

	case MathCx::Ln:
		output[0] = log(input[0]);
		break;

	case MathCx::Log10:
		output[0] = log10(input[0]);
		break;

	case MathCx::Pow10:
		output[0] = pow(10, input[0]);
		break;

	case MathCx::Recip:
		//output[0] = conj(input[0]) / pow(abs(input[0]), 2);
		output[0] = pow(input[0], -1);
		break;

	case MathCx::Round:
		output[0].real(round(input[0].real()));
		output[0].imag(round(input[0].imag()));
		break;

	case MathCx::Sqr:
		output[0] = pow(input[0], 2);
		break;

	case MathCx::Sqrt:
		output[0] = sqrt(input[0]);
		break;

	case MathCx::Conj:
		output[0] = conj(input[0]);
		break;

	default:
		break;
	}
	return true;
}
