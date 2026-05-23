#include "Inverse_M.h"
#include <sstream>

using namespace SystemVueModelBuilder;

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(Inverse_M)
{
	SET_MODEL_DESCRIPTION("Inverse Matrix Functon");   
	SET_MODEL_SYMBOL("SYM_Inverse_M");
	SET_MODEL_CATEGORY("Math Matrix");

	{
		DFPort port = ADD_MODEL_INPUT(input);
		port.SetName("input");
		port.SetDescription("input matrix (real)");
	}

	{
		DFPort port = ADD_MODEL_OUTPUT(output);
		port.SetName("output");
		port.SetDescription("inverse of input matrix (real)");
	}


	return true;
}
#endif  

Inverse_M::Inverse_M()
{
}

bool Inverse_M::Setup()
{
	input.SetRate(1U);
	output.SetRate(1U);
	return true;
}

bool Inverse_M::Run()
{
	const Matrix<double>& inMat = input[0];
	Matrix<double>&       outMat = output[0];

	if (!inMat.IsMatrix())
	{
		POST_ERROR("Inverse_M: input is not a matrix.");
		return false;
	}

	const size_t numRows = inMat.NumRows();
	const size_t numCols = inMat.NumColumns();

	if (numRows == 0 || numCols == 0 || numRows != numCols)
	{
		std::stringstream msg;
		msg << "Inverse_M: input matrix must be square and non-empty. "
			<< "Received (" << numRows << ", " << numCols << ").";
		POST_ERROR(msg.str().c_str());
		return false;
	}

	const bool ok = Matrix_Inverse<double>(inMat, outMat);
	if (!ok)
	{
		POST_ERROR("Inverse_M: the input matrix is singular and does not have an inverse.");
		return false;
	}

	return true;
}
