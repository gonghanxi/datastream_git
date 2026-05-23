#include "EnvToData.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(EnvToData)
{
	SET_MODEL_DESCRIPTION("Convert Envelope into its Characteristic Frequency, Time, Inphase and Quadrature Values");
	SET_MODEL_SYMBOL("SYM_EnvToData");
	SET_MODEL_CATEGORY("Analog/RF");

	ADD_MODEL_INPUT(input);
	ADD_MODEL_OUTPUT(fc);
	ADD_MODEL_OUTPUT(time);
	ADD_MODEL_OUTPUT(I);
	ADD_MODEL_OUTPUT(Q);
	return true;
}
#endif

ERESULT EnvToData::PropagateCharacterizationFrequency()
{
	return true;
}

EnvToData::EnvToData()
{
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool EnvToData::Run()
{
	I[0] = input[0].real();
	Q[0] = input[0].imag();
	time[0] = input.GetStartTime() + static_cast<double>(m_iFiringCount) * input.GetTimeStep();
	fc[0] = input.GetCharacterizationFrequency();

	return true;
}

