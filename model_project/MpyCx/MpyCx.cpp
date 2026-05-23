#include "MpyCx.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(MpyCx)
{
	SET_MODEL_DESCRIPTION("Multiple Input Multiplier (Datatype: complex)");
	SET_MODEL_SYMBOL("SYM_Mpy");
	SET_MODEL_CATEGORY("Math Matrix");
	SET_MODEL_CATEGORY("Math Scalar");

	ADD_MODEL_INPUT(input);
	ADD_MODEL_OUTPUT(output);
	return true;
}
#endif

MpyCx::MpyCx() {}

bool MpyCx::Setup()
{
	output.SetRate(1U);
	return true;
}

bool MpyCx::Run()
{
	cdouble acc = cdouble(1.0, 0.0);
	const std::size_t M = input.GetSize();
	for (std::size_t i = 0; i < M; ++i) {
		acc *= input[i][0U];
	}
	output[0U] = acc;
	return true;
}
