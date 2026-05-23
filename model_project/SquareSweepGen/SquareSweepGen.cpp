#include "SquareSweepGen.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( SquareSweepGen )
{	
	SET_MODEL_DESCRIPTION("Square Sweep Wave Generator");
	SET_MODEL_SYMBOL("SYM_SquareSweepGen");
	SET_MODEL_CATEGORY("Sources");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(frequency);
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
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(FSweepType, SelectedFSweepType);
		enumParam.SetDescription("Frequency sweep type: linear, log");
		enumParam.AddEnumeration("linear", linear);
		enumParam.AddEnumeration("log", log);
		enumParam.SetDefaultValue("0");
		enumParam.SetSchematicDisplay(0);
		enumParam.SetUseDefault(1);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(StartFreq);
		param.SetDescription("Start frequency");
		param.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		param.SetDefaultValue("1e3");
		param.SetSchematicDisplay(0);
		param.SetUseDefault(1);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(StopFreq);
		param.SetDescription("Stop frequency");
		param.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		param.SetDefaultValue("10e3");
		param.SetSchematicDisplay(0);
		param.SetUseDefault(1);
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
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(SweepPeriod);
		param.SetDescription("Time period to complete a sweep");
		param.SetUnit(SystemVueModelBuilder::Units::TIME);
		param.SetDefaultValue("Stop_Time + Time_Spacing");
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
	return true;
}
#endif

SquareSweepGen::SquareSweepGen()
{
	
}

bool SquareSweepGen::Setup()
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

	return bStatus;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool SquareSweepGen::Run()
{
	SampleRate = output.GetSampleRate();

	if (StartFreq <= 0 || StartFreq > SampleRate / 4)
	{
		POST_ERROR("StartFreq must be greater than 0 and smaller than SampleRate/4");
		return false;
	}

	if (StopFreq <= 0 || StopFreq > SampleRate / 4)
	{
		POST_ERROR("StopFreq must be greater than 0 and smaller than SampleRate/4");
		return false;
	}

	if (SweepPeriod < 1 / SampleRate)
	{
		POST_ERROR("SweepPeriod must be greater than 1/SampleRate");
		return false;
	}

	if (DutyCycle < 100 * std::max(StartFreq, StopFreq) / SampleRate || DutyCycle > 100 * (1 - std::max(StartFreq, StopFreq) / SampleRate))
	{
		POST_ERROR("DutyCycle must be greater than 100*max(StartFreq,StopFreq)/SampleRate and smaller than 100*(1-max(StartFreq,StopFreq)/SampleRate)");
		return false;
	}

	const double PI = std::acos(-1);
	double t = output.GetStartTime() + static_cast<double>(m_iFiringCount) * output.GetTimeStep() + 1e-16; // 神奇的浮点数误差修正

	// 初始时延部分置0
	if (t < InitialDelay)
	{
		output[0] = 0.0;
		frequency[0] = 0.0;
	}

	else
	{
		double tsweep = t - InitialDelay;
		double Fsweep = 0;
		switch (FSweepType)
		{
		case SquareSweepGen::linear:
			frequency[0] = StartFreq + (StopFreq - StartFreq)*(tsweep / SweepPeriod);
			Fsweep = StartFreq + (StopFreq - StartFreq)*(tsweep / SweepPeriod) / 2;
			break;
		case SquareSweepGen::log:
			frequency[0] = StartFreq * std::pow(StopFreq / StartFreq, tsweep / SweepPeriod);
			Fsweep = StartFreq * SweepPeriod / (tsweep*std::log(StopFreq / StartFreq))*(std::pow(StopFreq / StartFreq, tsweep / SweepPeriod) - 1.0);
			break;
		default:
			break;
		}

		double tEdge = DutyCycle / 100 / Fsweep;
		double PhaseD = std::fmod((std::fmod(Phase, 2 * PI) + 2 * PI), 2 * PI); // 相位映射至[0, 2*PI]的区间内
		double pt = std::fmod((t - InitialDelay + PhaseD / (2 * PI) / Fsweep), 1 / Fsweep); // 每个周期内对应时间

		if (pt < tEdge)
		{
			output[0] = Polarity ? LoLevel : HiLevel;
		}
		else
		{
			output[0] = Polarity ? HiLevel : LoLevel;
		}
	}

	return true;
}

