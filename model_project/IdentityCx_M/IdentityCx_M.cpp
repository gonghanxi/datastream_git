#include "IdentityCx_M.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( IdentityCx_M )
{	
	SET_MODEL_DESCRIPTION("Complex Identity Matrix Generator");
	SET_MODEL_SYMBOL("SYM_Identity_M");
	SET_MODEL_CATEGORY("Sources");
	SET_MODEL_CATEGORY("Math Matrix");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(RowsCols);
		param.SetDescription("Number of rows and columns in output square matrix");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("2");
	}

	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(ShowAdvancedParams, SelectedShowAdvancedParams);
		enumParam.SetDescription("Show advanced parameters: NO, YES");
		enumParam.AddEnumeration("NO", No);
		enumParam.AddEnumeration("YES", Yes);
		enumParam.SetDefaultValue("0");
		enumParam.SetSchematicDisplay(0);
		enumParam.SetUseDefault(1);
	}

	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(SampleRateOption, SelectedSampleRateOption);
		enumParam.SetDescription("Sample rate option: UnTimed, Timed from SampleRate, Timed from Schematic");
		enumParam.AddEnumeration("UnTimed", UnTimed);
		enumParam.AddEnumeration("Timed from SampleRate", TimedFromSampleRate);
		enumParam.AddEnumeration("Timed from Schematic", TimedFromSchematic);
		enumParam.SetDefaultValue("2");
		enumParam.SetHideCondition("ShowAdvancedParams ~= 1");
		enumParam.SetSchematicDisplay(0);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(SampleRate);
		param.SetDescription("Explicit sample rate");
		param.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		param.SetDefaultValue("Sample_Rate");
		param.SetHideCondition("SampleRateOption ~= 1 || ShowAdvancedParams ~= 1"); // 仅为直接设置采样率的选项提供这个参数
		param.SetSchematicDisplay(0);
		param.SetUseDefault(1);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(InitialDelay);
		param.SetDescription("Output sample delay");
		param.SetDefaultValue("0");
		param.SetHideCondition("ShowAdvancedParams ~= 1");
		param.SetSchematicDisplay(0);
		param.SetUseDefault(1);
	}
	return true;
}
#endif

IdentityCx_M::IdentityCx_M()
{
	
}

bool IdentityCx_M::Setup()
{
	bool bStatus = true;

	if (RowsCols < 1)
	{
		POST_ERROR("RowsCols must be >= 1.");
        LOG_ERROR("RowsCols must be >= 1.");
		bStatus = 0;
	}

	if (InitialDelay < 0)
	{
		POST_ERROR("InitialDelay must be >= 0.");
        LOG_ERROR("InitialDelay must be >= 0.");
		bStatus = 0;
	}
	/// 在计时模型中如何设置非计时端口尚待研究
	if (SampleRateOption == UnTimed)
	{
		POST_WARNING("Untimed sample is not supported yet. Output index may still be time related.");
        LOG_WARN("Untimed sample is not supported yet. Output index may still be time related.");
	}

	// 设置采样率
	if (SampleRateOption == TimedFromSampleRate)
	{
		if (SampleRate > 0)
		{
			// Use TimedCircularBuffer::SetSampleRate method to set the output sample rate
			output.SetSampleRate(SampleRate);
		}
		else
		{
			POST_ERROR("SampleRate must be greater than 0.");
            LOG_ERROR("SampleRate must be greater than 0.");
			bStatus = false;
		}
	}

	return bStatus;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool IdentityCx_M::Run()
{
	int i = GetCount();
	output[0].Resize(RowsCols, RowsCols);

	if (i < InitialDelay)
	{
		output[0].Zero();
	}
	else
	{
		for (int m = 0; m < RowsCols; m++)
		{
			for (int n = 0; n < RowsCols; n++)
			{
				output[0](m, n) = (m == n) ? 1 : 0;
			}
		}
	}
	return true;
}
