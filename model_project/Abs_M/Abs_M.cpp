#include "Abs_M.h"
#include <cmath>

using namespace SystemVueModelBuilder;

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(Abs_M)
{
	SET_MODEL_DESCRIPTION("Absolute Value Matrix Function");
	SET_MODEL_SYMBOL("SYM_Abs");
	SET_MODEL_CATEGORY("Math Matrix");

	{
		DFPort p = ADD_MODEL_INPUT(input);
		p.SetDescription("input matrix (real)");
	}
	{
		DFPort p = ADD_MODEL_OUTPUT(output);
		p.SetDescription("output matrix (element-wise absolute value)");
	}

	return true;
}
#endif  

bool Abs_M::Initialize()
{
	input.SetRate(1U);
	output.SetRate(1U);
	return true;
}

bool Abs_M::Run()
{
	Matrix<double>& inMat = input[0];
	Matrix<double>& outMat = output[0];

	if (!outMat.IsSizeEqual(inMat))
	{
		outMat.ResizeMultidimensional(inMat.NumDimensions(),
			inMat.Dimensions());
	}

	const size_t n = inMat.NumElements();

	for (size_t i = 0; i < n; ++i)
	{
		double v = inMat(i);
		if (v < 0.0) v = -v;   
		outMat(i) = v;
	}

	return true;
}

bool Abs_M::Finalize()
{
	return true;
}
