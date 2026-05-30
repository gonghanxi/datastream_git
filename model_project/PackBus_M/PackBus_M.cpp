#include "PackBus_M.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( PackBus_M )
{	
	SET_MODEL_DESCRIPTION("Bus to Matrix Converter");
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

PackBus_M::PackBus_M()
{
	
}

bool PackBus_M::Setup()
{
	bool bStatus = true;

	if (NumRows < 1 || NumCols < 1)
	{
		POST_ERROR("NumRows and NumCols must be >= 1.");
		bStatus = false;
	}
	if (NumRows*NumCols != input.GetSize())
	{
		POST_ERROR("input bus size must be equal to NumRows * NumCols.");
		bStatus = false;
	}

	return bStatus;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool PackBus_M::Run()
{
	output[0].Resize(NumRows, NumCols);

	for (int m = 0; m < NumRows; m++)
	{
		for (int n = 0; n < NumCols; n++)
		{
			int inputIndex = Format ? m * NumCols + n : n * NumRows + m;
			output[0](m, n) = input[inputIndex][0];
		}
	}
	return true;
}
