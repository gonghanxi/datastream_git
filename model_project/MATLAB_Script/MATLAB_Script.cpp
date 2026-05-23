#include "MATLAB_Script.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(MATLAB_Script)
{
	SET_MODEL_DESCRIPTION("Multiple Input Adder (Datatype: complex)");
	SET_MODEL_SYMBOL("SYM_Add");
	SET_MODEL_CATEGORY("Math Matrix");
	SET_MODEL_CATEGORY("Math Scalar");

	ADD_MODEL_INPUT(input);   // ∂‡ ‰»Î
	ADD_MODEL_OUTPUT(output);
	return true;
}
#endif

MATLAB_Script::MATLAB_Script() {}

bool MATLAB_Script::Setup()
{
	output.SetRate(1U);
	return true;
}

bool MATLAB_Script::Run()
{
	cdouble acc(0.0, 0.0);
	const std::size_t M = input.GetSize();
	for (std::size_t i = 0; i < M; ++i) {
		acc += input[i][0U];
	}
	output[0U] = acc;
	return true;
}
