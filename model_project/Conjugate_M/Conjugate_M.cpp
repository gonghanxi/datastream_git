#include "Conjugate_M.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(Conjugate_M)
{
	SET_MODEL_DESCRIPTION("Conjugate Matrix Function");
	SET_MODEL_SYMBOL("SYM_Conjugate_M");
	SET_MODEL_CATEGORY("Math Matrix");

	ADD_MODEL_INPUT(input);
	ADD_MODEL_OUTPUT(output);

	return true;
}
#endif

Conjugate_M::Conjugate_M()
{
}

bool Conjugate_M::Setup()
{
	input.SetRate(1U);
	output.SetRate(1U);
	return true;
}

bool Conjugate_M::Run()
{
	const SystemVueModelBuilder::Matrix< std::complex<double> >& inMx = input[0U];

	SystemVueModelBuilder::Matrix< std::complex<double> >& outMx = output[0U];
	outMx.Resize(inMx.NumRows(), inMx.NumColumns());

	const std::size_t N = inMx.NumElements();
	for (std::size_t i = 0; i < N; ++i) {
		const std::complex<double>& v = inMx(i);
		outMx(i) = std::complex<double>(v.real(), -v.imag());
	}

	return true;
}
