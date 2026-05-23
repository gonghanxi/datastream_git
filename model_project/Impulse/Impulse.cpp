#include "Impulse.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( Impulse )
{	
	SET_MODEL_DESCRIPTION("Impulse Waveform");
	SET_MODEL_SYMBOL("SYM_Impulse");
	SET_MODEL_CATEGORY("Sources");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(Level);
		param.SetDescription("Impulse level");
		param.SetUnit(SystemVueModelBuilder::Units::VOLTAGE);
		param.SetDefaultValue("1");
	}

	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(ScaleBySampleRate, SelectedNoOrYes);
		enumParam.SetDescription("Scale impulse level by sample rate: NO, YES");
		enumParam.AddEnumeration("NO", No);
		enumParam.AddEnumeration("YES", Yes);
		enumParam.SetDefaultValue("0");
	}

	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(ShowAdvancedParams, SelectedNoOrYes);
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
		param.SetUnit(SystemVueModelBuilder::Units::TIME);
		param.SetDefaultValue("0");
		param.SetHideCondition("ShowAdvancedParams ~= 1");
		param.SetSchematicDisplay(0);
		param.SetUseDefault(1);
	}

	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(BurstMode, SelectedBurstMode);
		enumParam.SetDescription("Burst mode: OFF, Single, Multiple");
		enumParam.AddEnumeration("OFF", OFF);
		enumParam.AddEnumeration("Single", Single);
		enumParam.AddEnumeration("Multiple", Multiple);
		enumParam.SetDefaultValue("0");
		enumParam.SetHideCondition("ShowAdvancedParams ~= 1");
		enumParam.SetSchematicDisplay(0);
		enumParam.SetUseDefault(1);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(BurstLength);
		param.SetDescription("Burst sample length");
		param.SetUnit(SystemVueModelBuilder::Units::TIME);
		param.SetDefaultValue("100e-6");
		param.SetHideCondition("ShowAdvancedParams ~= 1 || BurstMode == 0");
		param.SetSchematicDisplay(0);
		param.SetUseDefault(1);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(BurstPeriod);
		param.SetDescription("Samples from start of one burst to start of next");
		param.SetUnit(SystemVueModelBuilder::Units::TIME);
		param.SetDefaultValue("200e-6");
		param.SetHideCondition("ShowAdvancedParams ~= 1 || BurstMode ~= 2");
		param.SetSchematicDisplay(0);
		param.SetUseDefault(1);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(BurstDelay);
		param.SetDescription("Sample delay within burst before the start of the burst length interval");
		param.SetUnit(SystemVueModelBuilder::Units::TIME);
		param.SetDefaultValue("0");
		param.SetHideCondition("ShowAdvancedParams ~= 1 || BurstMode == 0");
		param.SetSchematicDisplay(0);
		param.SetUseDefault(1);
	}
	return true;
}
#endif

Impulse::Impulse()
{
	
}

bool Impulse::Setup()
{
	bool bStatus = true;

	/// 在计时模型中如何设置非计时端口尚待研究
	if (SampleRateOption == UnTimed)
	{
		POST_WARNING("Untimed sample is not supported yet. Output index may still be time related.");
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
			bStatus = false;
		}
	}

	if (InitialDelay < 0)
	{
		POST_ERROR("InitialDelay must not be negtive.");
		bStatus = false;
	}

	if (BurstDelay < 0 || BurstDelay > BurstPeriod - BurstLength)
	{
		POST_ERROR("BurstDelay must be greater than 0 and less than BurstPeriod - BurstLength");
		bStatus = false;
	}

	return bStatus;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool Impulse::Run()
{
	SampleRate = output.GetSampleRate();

	if (BurstLength < 1 / SampleRate)
	{
		POST_ERROR("BurstLength must be greater than 1 / SampleRate.");
		return false;
	}

	if (BurstPeriod < 1 / SampleRate)
	{
		POST_ERROR("BurstPeriod must be greater than 1 / SampleRate.");
		return false;
	}

	const double PI = std::acos(-1);
	double t = output.GetStartTime() + static_cast<double>(m_iFiringCount) * output.GetTimeStep();

	switch (BurstMode)
	{
	case Impulse::OFF:
	{
		if (std::abs(t - InitialDelay) < 0.5 / SampleRate)
		{
			output[0] = ScaleBySampleRate ? SampleRate : 1;
		}
		else
		{
			output[0] = 0;
		}
		break;
	}

	case Impulse::Single:
	{		
		if (std::abs(t - (InitialDelay + BurstDelay)) < 0.5 / SampleRate)
		{
			output[0] = ScaleBySampleRate ? SampleRate : 1;
		}
		else
		{
			output[0] = 0;
		}
		break;
	}

	case Impulse::Multiple:
	{
		double wt = fmod(t - InitialDelay, BurstPeriod); // 当前时间对应窗内相对时间
		if (std::abs(wt - BurstDelay) < 0.5 / SampleRate)
		{
			output[0] = ScaleBySampleRate ? SampleRate : 1;
		}
		else
		{
			output[0] = 0;
		}
		break;
	}
	default:
		break;
	}

	return true;
}

