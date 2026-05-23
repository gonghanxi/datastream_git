#include "PackCx_M.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( PackCx_M )
{	
	SET_MODEL_DESCRIPTION("Pack Matrix Function");
	SET_MODEL_SYMBOL("SYM_Pack_M");
	SET_MODEL_CATEGORY("Type Converters");
	SET_MODEL_CATEGORY("Math Matrix");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(input);
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(NumRows);
		param.SetDescription("Number of rows in output matrix");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("1");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(NumCols);
		param.SetDescription("Number of columns in output matrix");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("1");
	}

	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(Format, SelectedFormat);
		enumParam.SetDescription("Format of data to be packed into matrix: ColumnMajor, RowMajor");
		enumParam.AddEnumeration("ColumnMajor", ColumnMajor);
		enumParam.AddEnumeration("RowMajor", RowMajor);
		enumParam.SetDefaultValue("0");
	}
	return true;
}
#endif

PackCx_M::PackCx_M()
{
	
}

bool PackCx_M::Setup()
{
	bool bStatus = true;

	if (NumRows < 1 || NumCols < 1)
	{
		POST_ERROR("NumRows and NumCols must be >= 1.");
		bStatus = false;
	}
	else
	{
		input.SetRate(NumRows * NumCols);
	}

	return bStatus;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool PackCx_M::Run()
{
	output[0].Resize(NumRows, NumCols);

	for (int m = 0; m < NumRows; m++)
	{
		for (int n = 0; n < NumCols; n++)
		{
			int inputIndex = Format ? m * NumCols + n : n * NumRows + m;
			output[0](m, n) = input[inputIndex];
		}
	}
	return true;
}
