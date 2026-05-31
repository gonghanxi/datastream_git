#include "SubMxCx_M.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( SubMxCx_M )
{	
	SET_MODEL_DESCRIPTION("Submatrix Extractor");
	SET_MODEL_SYMBOL("SYM_SubMx_M");
	SET_MODEL_CATEGORY("Math Matrix");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(input);
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(StartRow);
		param.SetDescription("Starting row for the submatrix within the input matrix");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("1");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(StartCol);
		param.SetDescription("Starting column for the submatrix within the input matrix");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("1");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(NumRows);
		param.SetDescription("Number of rows in the output matrix");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("2");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(NumCols);
		param.SetDescription("Number of columns in the output matrix");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("2");
	}
	return true;
}
#endif

SubMxCx_M::SubMxCx_M()
{

}

bool SubMxCx_M::Setup()
{
	bool bStatus = true;

	if (StartRow < 1)
	{
		POST_ERROR("StartRow must be >= 1.");
		bStatus = false;
	}
	if (StartCol < 1)
	{
		POST_ERROR("StartCol must be >= 1.");
		bStatus = false;
	}
	if (NumRows < 1)
	{
		POST_ERROR("NumRows must be >= 1.");
		bStatus = false;
	}
	if (NumCols < 1)
	{
		POST_ERROR("NumCols must be >= 1.");
		bStatus = false;
	}

	return bStatus;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool SubMxCx_M::Run()
{
	output[0].Resize(NumRows, NumCols);

	if (StartRow + NumRows - 1 > input[0].NumRows())
	{
		POST_ERROR("Input matrix is too small or sub matrix is too large to extract. StartRow + NumRows - 1 must be <= inputRows");
		return false;
	}
    if (StartCol + NumCols - 1 > input[0].NumColumns())
	{
		POST_ERROR("Input matrix is too small or sub matrix is too large to extract. StartCol + NumCols - 1 must be <= inputCols");
		return false;
	}

	for (int m = 0; m < NumRows; m++)
	{
		for (int n = 0; n < NumCols; n++)
		{
			output[0](m, n) = input[0](StartRow + m - 1, StartCol + n - 1);
		}
	}
	return true;
}
