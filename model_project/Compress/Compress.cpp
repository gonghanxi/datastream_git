#include "Compress.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( Compress )
{	
	SET_MODEL_DESCRIPTION("Compression Part of a Compander");
	SET_MODEL_SYMBOL("SYM_Compress");
	SET_MODEL_CATEGORY("Signal Processing");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(input);
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
	}

	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(CompressionType, SelectedCompressionType);
		enumParam.SetDescription("Compression law: MU-law, A-law");
		enumParam.AddEnumeration("MU-law", MULaw);
		enumParam.AddEnumeration("A-law", ALaw);
		enumParam.SetDefaultValue("0");
		enumParam.SetSchematicDisplay(0);
		enumParam.SetUseDefault(1);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(CompressionK);
		param.SetDescription("Compression constant");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("1");
		param.SetSchematicDisplay(0);
		param.SetUseDefault(1);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(Max);
		param.SetDescription("Maximum input value magnitude");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("1");
		param.SetSchematicDisplay(0);
		param.SetUseDefault(1);
	}
	return true;
}
#endif

Compress::Compress()
{
	
}

bool Compress::Setup()
{
	bool bStatus = true;

	if (Max <= 0 || CompressionK <= 0)
	{
		POST_ERROR("Max and CompressionK must be greater than 0.");
        LOG_ERROR("Max and CompressionK must be greater than 0.");
		bStatus = false;
	}

	return bStatus;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool Compress::Run()
{
	double inputNorm = input[0] / Max;
	double inputSgn = inputNorm > 0 ? 1 : -1;

	if (CompressionType == Compress::MULaw)
	{
		output[0] = Max * (inputSgn * std::log(1.0 + CompressionK * std::abs(inputNorm))) / (std::log(1.0 + CompressionK));
	}

	else if (CompressionType == Compress::ALaw)
	{
		if (std::abs(inputNorm) < (1.0 / CompressionK))
		{
			output[0] = Max*(inputSgn * CompressionK*std::abs(inputNorm)) / (1.0 + std::log(CompressionK));
		}
		else if (std::abs(inputNorm) >= (1.0 / CompressionK))
		{
			output[0] = Max * (inputSgn * (1.0 + std::log(CompressionK*std::abs(inputNorm)))) / (1.0 + std::log(CompressionK));
		}
	}
	return true;
}
