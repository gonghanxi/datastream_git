#include "CxToEnv_M.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( CxToEnv_M )
{	
	SET_MODEL_DESCRIPTION("Complex Matrix to Envelope Matrix Converter");
	SET_MODEL_SYMBOL("SYM_CxToEnv");
	SET_MODEL_CATEGORY("Analog/RF");
	SET_MODEL_CATEGORY("Type Converters");

	ADD_MODEL_INPUT(input);

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(fc);
		port.SetOptional();
	}

	ADD_MODEL_OUTPUT(output);

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(Fc);
		param.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		param.SetDefaultValue("0.2e6");
	}
	return true;
}
#endif

CxToEnv_M::CxToEnv_M()
{

}

ERESULT CxToEnv_M::PropagateCharacterizationFrequency()
{
	bool bStatus = true;

	if (fc.IsConnected())
	{
		Fc = fc.GetCharacterizationFrequency();
	}

	if (Fc > 0)
	{
		output.SetCharacterizationFrequency(Fc);
	}
	else
	{
		POST_ERROR("characterization frequency must be greater than 0.");
		bStatus = false;
	}

	return bStatus;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool CxToEnv_M::Run()
{
	int NRow = input[0].NumRows();
	int NCol = input[0].NumColumns();
	output[0].Resize(NRow, NCol);

	for (int row = 0; row < NRow; row++)
	{
		for (int col = 0; col < NCol; col++)
		{
			output[0](row, col) = input[0](row, col);
		}
	}
	return true;
}
