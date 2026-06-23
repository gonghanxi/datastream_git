#include "ConstFxp.h"
#include <cmath>

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( ConstFxp )
{	
	{
		SET_MODEL_DESCRIPTION("Constant Generator");

		SET_MODEL_CATEGORY("Sources");

		{
			SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
		}

		{
			SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(Value);
			param.SetDescription("Value");
			param.SetUnit(SystemVueModelBuilder::Units::NONE);
			param.SetDefaultValue("0");
		}

		{
			SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(FxpPos);
			param.SetDescription("Fixpoint position");
			param.SetUnit(SystemVueModelBuilder::Units::NONE);
			param.SetDefaultValue("4");
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
			param.SetHideCondition("SampleRateOption ~= 1 || ShowAdvancedParams ~= 1"); // ��Ϊֱ�����ò����ʵ�ѡ���ṩ�������
			param.SetSchematicDisplay(0);
			param.SetUseDefault(1);
		}


		{
			SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(InitialDelay);
			param.SetDescription("Output sample delay");
			param.SetUnit(SystemVueModelBuilder::Units::NONE);
			param.SetDefaultValue("0");
			param.SetHideCondition("ShowAdvancedParams ~= 1");
			param.SetSchematicDisplay(0);
			param.SetUseDefault(1);
		}

		return true;
	}
}
#endif

ConstFxp::ConstFxp()
{
	
}

bool ConstFxp::Setup()
{
	bool bStatus = true;

	if (SampleRateOption == UnTimed)
	{
		POST_WARNING("Untimed sample is not supported yet. Output index may still be time related.");
	}

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
			bStatus = false;
		}
	}

	if (InitialDelay < 0)
	{
		POST_ERROR("InitialDelay must be >= 0");
		bStatus = false;
	}

	if (FxpPos < 0)
	{
		POST_ERROR("FxpPos must be >=0");
		bStatus = false;
	}

	return bStatus;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool ConstFxp::Run()
{
	int i = GetCount();
	if (i < InitialDelay)
	{
		output[0] = 0;
	}
	else
	{
		double factor = std::pow(10.0, FxpPos);
		output[0] = std::trunc(Value * factor) / factor;
	}
	return true;
}
