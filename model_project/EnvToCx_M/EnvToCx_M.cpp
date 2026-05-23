#include "EnvToCx_M.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( EnvToCx_M )
{	
	SET_MODEL_DESCRIPTION("Envelope Matrix to Complex Matrix Converter");
	SET_MODEL_SYMBOL("SYM_EnvToCx");
	SET_MODEL_CATEGORY("Analog/RF");
	SET_MODEL_CATEGORY("Type Converters");

	ADD_MODEL_INPUT(input);
	ADD_MODEL_OUTPUT(output);
	ADD_MODEL_OUTPUT(fc);
	return true;
}
#endif

EnvToCx_M::EnvToCx_M()
{
	
}

ERESULT EnvToCx_M::PropagateCharacterizationFrequency()
{
	fc.SetCharacterizationFrequency(input.GetCharacterizationFrequency());
	return true;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool EnvToCx_M::Run()
{
	int NRow = input[0].NumRows();
	int NCol = input[0].NumColumns();
	output[0].Resize(NRow, NCol);

	for (int row = 0; row < NRow; row++)
	{
		for (int col = 0; col < NCol; col++)
		{
			output[0](col, row) = input[0](col, row).complex();
		}
	}
	return true;
}
