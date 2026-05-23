#include "SquareGen.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( SquareGen )
{	
	SET_MODEL_DESCRIPTION("Square Wave Generator");
	SET_MODEL_SYMBOL("SYM_SquareGen");
	SET_MODEL_CATEGORY("Sources");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(LoLevel);
		param.SetDescription("Low voltage level");
		param.SetUnit(SystemVueModelBuilder::Units::VOLTAGE);
		param.SetDefaultValue("0");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(HiLevel);
		param.SetDescription("High voltage level");
		param.SetUnit(SystemVueModelBuilder::Units::VOLTAGE);
		param.SetDefaultValue("1");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(Frequency);
		param.SetDescription("Frequency");
		param.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		param.SetDefaultValue("5e3");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(Phase);
		param.SetDescription("Phase");
		param.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		param.SetDefaultValue("0");
		param.SetSchematicDisplay(0);
		param.SetUseDefault(1);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(DutyCycle);
		param.SetDescription("Duty cycle in percent");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("50");
		param.SetSchematicDisplay(0);
		param.SetUseDefault(1);
	}

	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(Polarity, SelectedPolarity);
		enumParam.SetDescription("Signal polarity: normal, inverted");
		enumParam.AddEnumeration("normal", normal);
		enumParam.AddEnumeration("inverted", inverted);
		enumParam.SetDefaultValue("0");
		enumParam.SetSchematicDisplay(0);
		enumParam.SetUseDefault(1);
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

SquareGen::SquareGen()
{

}

bool SquareGen::Setup()
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

	if (LoLevel >= HiLevel)
	{
		POST_ERROR("LoLevel must be < HiLevel");
		bStatus = false;
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
bool SquareGen::Run()
{
	SampleRate = output.GetSampleRate();

	if (Frequency < 0 || Frequency > SampleRate / 4)
	{
		POST_ERROR("Frequency must be greater than 0 and smaller than SampleRate / 4");
		return false;
	}

	if (DutyCycle < 100 * Frequency / SampleRate || DutyCycle > 100 * (1 - Frequency / SampleRate))
	{
		POST_ERROR("DutyCycle must be greater than 100*Frequency/SampleRate and smaller than 100*(1-Frequency/SampleRate)");
		return false;
	}

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
	double t = output.GetStartTime() + static_cast<double>(m_iFiringCount) * output.GetTimeStep() + 1e-16; // 神奇的浮点数误差修正;
	double tEdge = DutyCycle / 100 / Frequency;
	double PhaseD = std::fmod((std::fmod(Phase, 2 * PI) + 2 * PI), 2 * PI); // 相位映射至[0, 2*PI]的区间内
	double pt = std::fmod((t - InitialDelay + PhaseD / (2 * PI) / Frequency), 1 / Frequency); // 每个周期内对应时间

	switch (BurstMode)
	{
	case SquareGen::OFF:
	{
		if ( pt < tEdge)
		{
			output[0] = Polarity ? LoLevel : HiLevel;
		}
		else
		{
			output[0] = Polarity ? HiLevel : LoLevel;
		}
		break;
	}
	case SquareGen::Single:
	{
		if (t >= InitialDelay && t < InitialDelay + BurstDelay)
		{
			output[0] = Polarity ? HiLevel : LoLevel;
		}
		else if (t >= InitialDelay + BurstDelay && t < InitialDelay + BurstDelay + BurstLength)
		{
			if (pt - BurstDelay < tEdge)
			{
				output[0] = Polarity ? LoLevel : HiLevel;
			}
			else
			{
				output[0] = Polarity ? HiLevel : LoLevel;
			}
		}
		else
		{
			output[0] = 0;
		}
		break;
	}
	case SquareGen::Multiple:
	{
		double wt = std::fmod(t - InitialDelay - BurstDelay, BurstPeriod); // 多脉冲模式下每个脉冲周期内对应的时间

		if (t >= InitialDelay && t < InitialDelay + BurstDelay)
		{
			output[0] = Polarity ? HiLevel : LoLevel;
		}
		else if (wt <= BurstLength)
		{
			if (pt - BurstDelay < tEdge)
			{
				output[0] = Polarity ? LoLevel : HiLevel;
			}
			else
			{
				output[0] = Polarity ? HiLevel : LoLevel;
			}
		}
		else
		{
			output[0] = Polarity ? HiLevel : LoLevel;
		}
		break;
	}
	default:
		break;
	}

	// 初始时延置0
	if (t < InitialDelay)
	{
		output[0] = 0;
	}

	return true;
}

