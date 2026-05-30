#include "MxCom_M.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( MxCom_M )
{	
	SET_MODEL_DESCRIPTION("Matrix Composer");
	SET_MODEL_SYMBOL("SYM_MxCom_M");
	SET_MODEL_CATEGORY("Math Matrix");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(input);
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(OutputNumRows);
		param.SetDescription("Number of rows for output matrix");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("100");
		param.SetSchematicDisplay(0);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(OutputNumCols);
		param.SetDescription("Number of columns for output matrix");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("100");
		param.SetSchematicDisplay(0);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(InputNumRows);
		param.SetDescription("Number of rows for input matrix");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("4");
		param.SetSchematicDisplay(0);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(InputNumCols);
		param.SetDescription("Number of columns for input matrix");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("4");
		param.SetSchematicDisplay(0);
	}
	return true;
}
#endif

MxCom_M::MxCom_M()
{

}

bool MxCom_M::Setup()
{
	bool bStatus = true;

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
	if (InputNumRows < 1)
	{
		POST_ERROR("NumRows must be >= 1.");
		bStatus = false;
	}
	if (InputNumCols < 1)
	{
		POST_ERROR("NumCols must be >= 1.");
		bStatus = false;
	}
	if (OutputNumRows % InputNumRows)
	{
		POST_ERROR("OutputNumRows must be an integer multiple of InputNumRows.");
		bStatus = false;
	}
	if (OutputNumCols % InputNumCols)
	{
		POST_ERROR("OutputNumColumns must be an integer multiple of InputNumColumns.");
		bStatus = false;
	}

	input.SetRate(OutputNumRows / InputNumRows * OutputNumCols / InputNumCols);

	return bStatus;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool MxCom_M::Run()
{
	output[0].Resize(OutputNumRows, OutputNumCols);

	for (int m = 0; m < OutputNumRows; m++)
	{
		for (int n = 0; n < OutputNumCols; n++)
		{
			// 子矩阵在大矩阵内是按从左到右，从上到下的顺序平铺的
			int MxRowIndex = m / InputNumRows;		// 当前元素所在子矩阵在大矩阵中的行索引
			int MxColIndex = n / InputNumCols;		// 当前元素所在子矩阵在大矩阵中的列索引
			int MxInputIndex = MxRowIndex * OutputNumCols / InputNumCols + MxColIndex;	// 当前元素所在子矩阵在输入序列中的索引
			int SubMxRowIndex = m % InputNumRows;	// 当前元素在子矩阵中的行索引
			int SubMxColIndex = n % InputNumCols;	// 当前元素在子矩阵中的列索引

			input[MxInputIndex].Resize(InputNumRows, InputNumCols);
			output[0](m, n) = input[MxInputIndex](SubMxRowIndex, SubMxColIndex);
		}
	}
	return true;
}
