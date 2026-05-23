#include "BitDeformatter.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( BitDeformatter )
{	
	SET_MODEL_DESCRIPTION("NRZ/RZ Symbol to Bit Converter");
	SET_MODEL_SYMBOL("SYM_BitDeformatter");
	SET_MODEL_CATEGORY("Type Converters");
	SET_MODEL_CATEGORY("Signal Processing");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(input);
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(SamplesPerBit);
		param.SetDescription("Number of output samples per input bit");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("1");
	}

	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(Format, SelectedFormat);
		enumParam.SetDescription("Format for output signal: NRZ, RZ");
		enumParam.AddEnumeration("NRZ", NRZ);
		enumParam.AddEnumeration("RZ", RZ);
		enumParam.SetDefaultValue("0");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(LogicZeroLevel);
		param.SetDescription("Voltage for bit value zero");
		param.SetUnit(SystemVueModelBuilder::Units::VOLTAGE);
		param.SetDefaultValue("-1");
		param.SetSchematicDisplay(0);
		param.SetUseDefault(1);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(LogicOneLevel);
		param.SetDescription("Voltage for bit value one");
		param.SetUnit(SystemVueModelBuilder::Units::VOLTAGE);
		param.SetDefaultValue("1");
		param.SetSchematicDisplay(0);
		param.SetUseDefault(1);
	}
	return true;
}
#endif

BitDeformatter::BitDeformatter()
{

}

bool BitDeformatter::Setup()
{
	bool bStatus = true;

	if (SamplesPerBit >= 1)
	{
		input.SetRate(SamplesPerBit);
	}
	else
	{
		POST_ERROR("SamplesPerBit must be >= 1.");
        LOG_ERROR("SamplesPerBit must be >= 1.");
		bStatus = false;
	}

	if (Format == BitDeformatter::RZ&&SamplesPerBit % 2)
	{
		POST_ERROR("SamplesPerBit must be even for RZ Format.");
        LOG_ERROR("SamplesPerBit must be even for RZ Format.");
		bStatus = false;
	}

	return bStatus;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool BitDeformatter::Run()
{
	if (Format == BitDeformatter::NRZ)
	{
		double bitAverage = 0;
		for (int i = 0; i < SamplesPerBit; i++)
		{
			bitAverage += input[i];
		}
		bitAverage /= SamplesPerBit;

		output[0] = (std::abs(bitAverage - LogicZeroLevel) < std::abs(bitAverage - LogicOneLevel)) ? 0 : 1;
	}

	else if (Format == BitDeformatter::RZ)
	{
		double bitZero = 0;
		double bitAverage = 0;
		for (int i = 0; i < SamplesPerBit; i++)
		{
			if (i < SamplesPerBit / 2)
			{
				bitAverage += input[i];
			}
			else
			{
				bitZero += input[i];
			}
		}
		bitZero /= SamplesPerBit / 2;
		bitAverage /= SamplesPerBit / 2;
		bitAverage -= bitZero;

		output[0] = (std::abs(bitAverage - LogicZeroLevel) < std::abs(bitAverage - LogicOneLevel)) ? 0 : 1;
	}
	return true;
}
