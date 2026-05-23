#include "TransposeInt.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( TransposeInt )
{	
	SET_MODEL_DESCRIPTION("Data Transposer");
	SET_MODEL_SYMBOL("SYM_Transpose");
	SET_MODEL_CATEGORY("Signal Processing");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(input);
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(SamplesInRow);
		param.SetDescription("Number of input samples constituting a row");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("8");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(NumberOfRows);
		param.SetDescription("Number of rows in the input matrix");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("8");
	}
	return true;
}
#endif

TransposeInt::TransposeInt()
{

}

bool TransposeInt::Setup()
{
	bool bStatus = true;

	if (SamplesInRow >= 1 && NumberOfRows >= 1)
	{
		input.SetRate(SamplesInRow * NumberOfRows);
		output.SetRate(SamplesInRow * NumberOfRows);
	}
	else
	{
		POST_ERROR("SamplesInRow and NumberOfRows must not be smaller than 1.");
		bStatus = false;
	}

	return bStatus;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool TransposeInt::Run()
{
	for (int cols = 0; cols < SamplesInRow; cols++)
	{
		for (int rows = 0; rows < NumberOfRows; rows++)
		{
			output[cols*NumberOfRows + rows] = input[rows*SamplesInRow + cols];
		}
	}
	return true;
}
