#include "ToeplitzCx_M.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( ToeplitzCx_M )
{	
	SET_MODEL_DESCRIPTION("Toeplitz Matrix Converter");
	SET_MODEL_SYMBOL("SYM_Toeplitz_M");
	SET_MODEL_CATEGORY("Math Matrix");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(input);
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
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

ToeplitzCx_M::ToeplitzCx_M()
{
	
}

bool ToeplitzCx_M::Setup()
{
	bool bStatus = true;

	if (NumRows > 0 && NumCols > 0)
	{
		input.SetRate(NumRows + NumCols - 1);
	}
	else
	{
		POST_ERROR("NumRows and NumCols must be greater than 0.");
		bStatus = false;
	}

	return bStatus;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool ToeplitzCx_M::Run()
{
	output[0].Resize(NumRows, NumCols);

	for (int m = 0; m < NumRows; m++)
	{
		for (int n = 0; n < NumCols; n++)
		{
			output[0](m, n) = input[NumCols - n + m - 1];
		}
	}
	return true;
}
