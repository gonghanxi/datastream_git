#include "FFT_Shift.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( FFT_Shift )
{	
	SET_MODEL_DESCRIPTION("Swap Left and Right Halves of FFT Output or IFFT Input");
	SET_MODEL_SYMBOL("SYM_FFT_Shift@Data Flow Symbols");
	SET_MODEL_CATEGORY("Signal Processing");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(input);
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(FFTSize);
		param.SetDescription("FFT or IFFT size");
		param.SetDefaultValue("256");
	}

	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(Direction, SelectedDirection);
		enumParam.SetDescription("FFT shift or IFFT shift");
		enumParam.AddEnumeration("FFTShift", FFTShift);
		enumParam.AddEnumeration("IFFTShift", IFFTShift);
		enumParam.SetDefaultValue("0");
	}
	return true;
}
#endif

FFT_Shift::FFT_Shift()
{
	
}

bool FFT_Shift::Setup()
{
	bool bStatus = true;

	if (FFTSize >= 1)
	{
		input.SetRate(FFTSize);
		output.SetRate(FFTSize);
	}

	else
	{
		POST_ERROR("FFTSize should be greater than 1");
		bStatus = false;
	}

	return bStatus;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool FFT_Shift::Run()
{
	// FFT Shift 是向右圆周位移 FFTSize/2（向下取整）位
	if (Direction == FFT_Shift::FFTShift)
	{
		for (int i = 0; i < FFTSize; i++)
		{
			int n = i - FFTSize / 2;

			output[i] = input[n >= 0 ? n : n + FFTSize];
		}
	}

	// IFFT Shift是向左圆周位移 FFTSize/2（向下取整）位
	else if (Direction == FFT_Shift::IFFTShift)
	{
		for (int i = 0; i < FFTSize; i++)
		{
			int n = i + FFTSize / 2;

			output[i] = input[n < FFTSize ? n : n - FFTSize];
		}
	}
	return true;
}
