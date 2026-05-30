#include "Limit_M.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( Limit_M )
{	
	SET_MODEL_DESCRIPTION("Limiter");
	SET_MODEL_SYMBOL("SYM_Limit");
	SET_MODEL_CATEGORY("Math Matrix");
	// SET_MODEL_CATEGORY("Signal Processing");

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

Limit_M::Limit_M()
{

}

bool Limit_M::Setup()
{
	bool bStatus = true;

	if (K == 0)
	{
		POST_ERROR("K must not be 0.");
		bStatus = false;
	}

	if (Bottom > Top)
	{
		POST_ERROR("Top must be > Bottom.");
		bStatus = false;
	}

	return bStatus;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool Limit_M::Run()
{
	const double PI = std::acos(-1);

	int numCol = input[0].NumColumns();
	int numRow = input[0].NumRows();
	output[0].Resize(numRow, numCol);

	for (int i = 0; i < input[0].NumElements(); i++)
	{
		if (LimiterType == Limit_M::linear)
		{
			if (input[0](i) < Bottom / K)
			{
				output[0](i) = Bottom;
			}
			else if (input[0](i) > Top / K)
			{
				output[0](i) = Top;
			}
			else
			{
				output[0](i) = K * input[0](i);
			}
		}

		if (LimiterType == Limit_M::atan)
		{
			// SystemVue文档里给的公式是错的，以下面这个公式为准
			output[0](i) = (Top - Bottom) / PI * std::atan(PI*(K*input[0](i) - (Top + Bottom) / 2) / (Top - Bottom)) + (Top + Bottom) / 2.0;
		}
	}
	return true;
}
