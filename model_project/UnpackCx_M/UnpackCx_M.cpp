#include "UnpackCx_M.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( UnpackCx_M )
{	
	SET_MODEL_DESCRIPTION("Unpack Matrix Function");
	SET_MODEL_SYMBOL("SYM_UnPk_M");
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
		param.SetDescription("Number of rows in input matrix");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("1");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(NumCols);
		param.SetDescription("Number of columns in input matrix");
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

UnpackCx_M::UnpackCx_M()
{

}

bool UnpackCx_M::Setup()
{
	bool bStatus = true;

	if (NumRows < 1 || NumCols < 1)
	{
		POST_ERROR("NumRows and NumCols must be >= 1.");
		bStatus = false;
	}
	else
	{
		output.SetRate(NumRows * NumCols);
	}

	return bStatus;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool UnpackCx_M::Run()
{
	int InRows = input[0].NumRows();
	int InCols = input[0].NumColumns();

	// 此处是按SystemVue说明文档里的方式补零，SystemVue模型的实际补零存在问题
	for (int m = 0; m < NumRows; m++)
	{
		for (int n = 0; n < NumCols; n++)
		{
			int outputIndex = Format ? m * NumCols + n : n * NumRows + m;

			output[outputIndex] = (m < InRows && n < InCols) ? input[0](m, n) : 0;
		}
	}
	return true;
}
