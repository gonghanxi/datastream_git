#include "MxDecom_M.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( MxDecom_M )
{	
	SET_MODEL_DESCRIPTION("Matrix Decomposer");
	SET_MODEL_SYMBOL("SYM_MxDecom_M");
	SET_MODEL_CATEGORY("Math Matrix");
	
	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(input);
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(StartRow);
		param.SetDescription("Starting row in input matrix to generate output matrices (first row is 1)");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("1");
		param.SetSchematicDisplay(0);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(StartCol);
		param.SetDescription("Starting column in input matrix to generate output matrices (first column is 1, matrix upper left corner is (1,1)");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("1");
		param.SetSchematicDisplay(0);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(InputNumRows);
		param.SetDescription("Number of rows for input matrix");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("100");
		param.SetSchematicDisplay(0);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(InputNumCols);
		param.SetDescription("Number of columns from input matrix");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("100");
		param.SetSchematicDisplay(0);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(OutputNumRows);
		param.SetDescription("Number of rows for output matrix");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("4");
		param.SetSchematicDisplay(0);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(OutputNumCols);
		param.SetDescription("Number of columns for output matrix");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("4");
		param.SetSchematicDisplay(0);
	}
	return true;
}
#endif

MxDecom_M::MxDecom_M()
{

}

bool MxDecom_M::Setup()
{
	bool bStatus = true;

	if (StartRow < 1)
	{
		POST_ERROR("StartRow must be >= 1");
		bStatus = false;
	}
	if (StartCol < 1)
	{
		POST_ERROR("StartCol must be >= 1");
		bStatus = false;
	}
	if (InputNumRows < 1)
	{
		POST_ERROR("InputNumRows must be >= 1.");
		bStatus = false;
	}
	if (InputNumCols < 1)
	{
		POST_ERROR("InputNumCols must be >= 1.");
		bStatus = false;
	}
	if (OutputNumRows < 1)
	{
		POST_ERROR("OutputNumRows must be >= 1.");
		bStatus = false;
	}
	if (OutputNumCols < 1)
	{
		POST_ERROR("OutputNumColumns must be >= 1.");
		bStatus = false;
	}
	if (InputNumRows % OutputNumRows)
	{
		POST_ERROR("InputNumRows must be an integer multiple of OutputNumRows.");
		bStatus = false;
	}
	if (InputNumCols % OutputNumCols)
	{
		POST_ERROR("InputNumColumns must be an integer multiple of OutputNumColumns.");
		bStatus = false;
	}

	output.SetRate(InputNumRows / OutputNumRows * InputNumCols / OutputNumCols);

	return bStatus;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool MxDecom_M::Run()
{
	bool bStatus = true;

	if (input[0].NumRows() < StartRow + InputNumRows - 1)
	{
		POST_ERROR("Input matrix is too small. Rows of input matrix must >= StartRow + InputNumRows - 1");
		bStatus = false;
		
	}
	if (input[0].NumColumns() < StartCol + InputNumCols - 1)
	{
		POST_ERROR("Input matrix is too small. Columns of input matrix must >= StartCol + InputNumCols - 1");
		bStatus = false;
	}


	for (int m = 0; m < InputNumRows; m++)
	{
		for (int n = 0; n < InputNumCols; n++)
		{
			// 子矩阵在大矩阵内是按从左到右，从上到下的顺序平铺的
			int MxRowIndex = m / OutputNumRows;		// 当前元素所在子矩阵在大矩阵中的行索引
			int MxColIndex = n / OutputNumCols;		// 当前元素所在子矩阵在大矩阵中的列索引
			int MxOutputIndex = MxRowIndex * InputNumCols / OutputNumCols + MxColIndex;	// 当前元素所在子矩阵在输出序列中的索引
			int SubMxRowIndex = m % OutputNumRows;	// 当前元素在子矩阵中的行索引
			int SubMxColIndex = n % OutputNumCols;	// 当前元素在子矩阵中的列索引

			output[MxOutputIndex].Resize(OutputNumRows, OutputNumCols);
			output[MxOutputIndex](SubMxRowIndex, SubMxColIndex)= input[0](m + StartRow - 1, n + StartCol - 1);
		}
	}
	return bStatus;
}
