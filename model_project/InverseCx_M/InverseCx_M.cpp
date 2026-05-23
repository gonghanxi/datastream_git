#include "InverseCx_M.h"
#include <sstream>

using namespace SystemVueModelBuilder;

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(InverseCx_M)
{
	SET_MODEL_DESCRIPTION("Complex Inverse Matrix Function");
	SET_MODEL_SYMBOL("SYM_Inverse_M");
	SET_MODEL_CATEGORY("Math Matrix");

	{
		DFPort port = ADD_MODEL_INPUT(input);
		port.SetName("input");
		port.SetDescription("input complex matrix");
	}

	{
		DFPort port = ADD_MODEL_OUTPUT(output);
		port.SetName("output");
		port.SetDescription("inverse complex matrix");
	}

	return true;
}
#endif  

bool InverseCx_M::Run()
{
	DComplexMatrix& inMat = input[0];
	DComplexMatrix& outMat = output[0];

	if (!inMat.IsMatrix())
	{
		POST_ERROR("Input is not a matrix.");
		return false;
	}

	const size_t numRows = inMat.NumRows();
	const size_t numCols = inMat.NumColumns();

	if (numRows == 0 || numCols == 0)
	{
		POST_ERROR("Input matrix must not be empty.");
		return false;
	}

	if (numRows != numCols)
	{
		std::stringstream msg;
		msg << "Input matrix must be square.  Received matrix that was ("
			<< numRows << ", " << numCols << ").";
		POST_ERROR(msg.str().c_str());
		return false;
	}

	outMat.Resize(numRows, numCols);

	if (!Matrix_Inverse<std::complex<double>>(inMat, outMat))
	{
		POST_ERROR("The input matrix is singular and does not have an inverse.");
		return false;
	}

	return true;
}
