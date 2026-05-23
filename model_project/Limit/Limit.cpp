#include "Limit.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( Limit )
{	
	SET_MODEL_DESCRIPTION("Limiter");
	SET_MODEL_SYMBOL("SYM_Limit");
	SET_MODEL_CATEGORY("Signal Processing");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(input);
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(K);
		param.SetDescription("Magnitude gain");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("1");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(Bottom);
		param.SetDescription("Lower output saturation value");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("0");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(Top);
		param.SetDescription("Higher output saturation value");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("1");
	}

	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(LimiterType, SelectedLimiterType);
		enumParam.SetDescription("Type of limiting curve: linear, atan");
		enumParam.AddEnumeration("linear", linear);
		enumParam.AddEnumeration("atan", atan);
		enumParam.SetDefaultValue("0");
	}
	return true;
}
#endif

Limit::Limit()
{

}

bool Limit::Setup()
{
	bool bStatus = true;
	
	if (K == 0)
	{
		POST_ERROR("K must not be 0.");
        LOG_ERROR("K must not be 0.");
		bStatus = false;
	}

	if (Bottom > Top)
	{
		POST_ERROR("Top must be > Bottom.");
        LOG_ERROR("Top must be > Bottom.");
		bStatus = false;
	}
	
	return bStatus;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool Limit::Run()
{
	const double PI = std::acos(-1);

	if (LimiterType == Limit::linear)
	{
		if (input[0] < Bottom / K)
		{
			output[0] = Bottom;
		}
		else if (input[0] > Top / K)
		{
			output[0] = Top;
		}
		else
		{
			output[0] = K * input[0];
		}
	}

	if (LimiterType == Limit::atan)
	{
		// SystemVue文档里给的公式是错的，以下面这个公式为准
		output[0] = (Top - Bottom) / PI * std::atan(PI*(K*input[0] - (Top + Bottom) / 2) / (Top - Bottom)) + (Top + Bottom) / 2.0;
	}
	return true;
}
