#include "PolynomialCx.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( PolynomialCx )
{	
	SET_MODEL_DESCRIPTION("Polynomial Function");
	SET_MODEL_SYMBOL("SYM_Polynomial");
	SET_MODEL_CATEGORY("Signal Processing");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(input);
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(Coefficients);
		param.SetDescription("Polynomial coefficients (0-th order coefficient first)");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("[0 1]");
		param.SetUseDefault(1);
		param.SetSchematicDisplay(0);
	}
	return true;
}
#endif

PolynomialCx::PolynomialCx()
{

}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool PolynomialCx::Run()
{
	int order = Coefficients.NumElements();
	std::complex<double>	result = 0.0;
	std::complex<double>	term = 1.0; // 0次项

	// 阶数从低到高，依次累加各项
	for (int i = 0; i < order; i++)
	{
		result += Coefficients(i) * term;
		term *= input[0];
	}

	output[0] = result;
	return true;
}
