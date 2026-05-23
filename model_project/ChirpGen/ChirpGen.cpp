#include "ChirpGen.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( ChirpGen )
{	
	SET_MODEL_DESCRIPTION("Frequency Chirp Generator");
	SET_MODEL_SYMBOL("SYM_SineSweepGen");
	SET_MODEL_CATEGORY("Sources");

	ADD_MODEL_OUTPUT(SigOutput);	// double-蓝色箭头
	ADD_MODEL_OUTPUT(freqOutput);	// double-蓝色箭头

	// 参数：Amplitude
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(Amplitude);
		p.SetUnit(SystemVueModelBuilder::Units::VOLTAGE);
		p.SetDefaultValue("1");
		p.SetDescription("Peak sine wave voltage amplitude");
	}

	// 参数：Offset
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(Offset);
		p.SetUnit(SystemVueModelBuilder::Units::VOLTAGE);
		p.SetDefaultValue("0");
		p.SetDescription("DC offset voltage");
	}

	// 参数：StartFreq
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(StartFreq);
		p.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		p.SetDefaultValue("1e3");
		p.SetDescription("Start frequency");
		p.SetSchematicDisplay(0);
	}

	// 参数：StopFreq
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(StopFreq);
		p.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		p.SetDefaultValue("10e3");
		p.SetDescription("Stop frequency");
		p.SetSchematicDisplay(0);
	}

	// 参数：Phase
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(Phase);
		p.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		p.SetDefaultValue("0");
		p.SetDescription("Phase");
		p.SetSchematicDisplay(0);
	}

	// 参数：SweepPeriod
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(SweepPeriod);
		p.SetUnit(SystemVueModelBuilder::Units::TIME);
		p.SetDefaultValue("Stop_Time + Time_Spacing");
		p.SetDescription("Time period to complete a sweep");
		p.SetSchematicDisplay(0);
	}

	// 参数：ShowAdvancedParams（枚举）
	{
		SystemVueModelBuilder::DFParam e =
			ADD_MODEL_ENUM_PARAM(ShowAdvancedParams, ShowAdvancedParamsEnum);
		e.SetUnit(SystemVueModelBuilder::Units::NONE);
		e.AddEnumeration("NO", NO);
		e.AddEnumeration("YES", YES);
		e.SetDefaultValue("NO");
		e.SetDescription("whether show advanced parameters");
		e.SetSchematicDisplay(0);
	}

	// 参数：SampleRateOption（枚举）
	{
		SystemVueModelBuilder::DFParam e =
			ADD_MODEL_ENUM_PARAM(SampleRateOption, SampleRateOptionEnum);
		e.SetUnit(SystemVueModelBuilder::Units::NONE);
		e.AddEnumeration("UnTimed", UnTimed);
		e.AddEnumeration("Timed from SampleRate", TimedfromSampleRate);
		e.AddEnumeration("Timed from Schematic", TimedfromSchematic);
		e.SetDefaultValue("Timed from Schematic");
		e.SetDescription("Sample rate option");
		e.SetHideCondition("ShowAdvancedParams ~= 1");
		e.SetSchematicDisplay(0);
	}

	// 参数：SampleRate
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(SampleRate);
		p.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		p.SetDefaultValue("Sample_Rate");
		p.SetDescription("Explicit sample rate");
		p.SetHideCondition("SampleRateOption ~= 0 && SampleRateOption ~= 1 || ShowAdvancedParams ~= 1");
		p.SetSchematicDisplay(0);
		p.SetUseDefault(true);
	}

	// 参数：InitialDelay
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(InitialDelay);
		p.SetUnit(SystemVueModelBuilder::Units::TIME);
		p.SetDefaultValue("0");
		p.SetDescription("Initial output time delay");
		p.SetHideCondition("ShowAdvancedParams ~= 1");
		p.SetSchematicDisplay(0);
	}

	return true;
}
#endif

ChirpGen::ChirpGen()
{
	
}

bool ChirpGen::Setup()
{
	bool bStatus = true;

	if (StartFreq <= 0)
	{
		POST_ERROR("StartFreq must be positive.");
		bStatus = false;
	}

	if (StopFreq <= 0)
	{
		POST_ERROR("StopFreq must be positive.");
		bStatus = false;
	}

	if (StartFreq > SampleRate / 4)
	{
		POST_ERROR("StartFreq must <= SampleRate / 4, please either increase SampleRate or decrease StartFreq");
		bStatus = false;
	}

	if (StopFreq > SampleRate / 4)
	{
		POST_ERROR("StopFreq must <= SampleRate / 4, please either increase SampleRate or decrease StopFreq");
		bStatus = false;
	}

	if (SweepPeriod < 1 / SampleRate)
	{
		POST_ERROR("SweepPeriod must be larger than 1 / SampleRate");
		bStatus = false;
	}

	//在计时模型中如何设置非计时端口尚待研究
	if (SampleRateOption == UnTimed)
		POST_WARNING("Untimed sample is not supported yet. Output index may stil1 be time related.");

	// 设置采样率
	if (SampleRateOption == TimedfromSampleRate)
	{
		if (SampleRate > 0)
		{
			//Use TimedCircularBuffer::SetSampleRate method to set the output sample rate
			SigOutput.SetSampleRate(SampleRate);
		}
		else
		{
			POST_ERROR("SampleRate must be greater than 0.");
			bStatus = false;
		}
	}

	if (InitialDelay < 0)
	{
		POST_ERROR("InitialDelay must be positive.");
		bStatus = false;
	}

	return bStatus;
}


//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool ChirpGen::Run()
{
	const double PI = 3.14159265358979323846;
	double t = counter / SampleRate; // 当前时间
	static double freq = StartFreq; // 当前频率
	double chirpRate = (StopFreq - StartFreq) / SweepPeriod; // 频率变化斜率

	// 初始延迟处理
	if (ShowAdvancedParams == YES && InitialDelay > 0)
	{
		if (t < InitialDelay)
		{
			SigOutput[0] = 0;
			freqOutput[0] = 0;
			counter++;
			return true;
		}
		else
		{
			t = t - InitialDelay;
		}
	}

	// 计算当前时间在扫频周期内的位置
	double t_Period = fmod(t, SweepPeriod);

	// 计算瞬时扫频频率
	double F1_t = StartFreq + chirpRate * t_Period;

	// 计算平均扫频频率
	double F_t = StartFreq + 0.5 * chirpRate * t_Period;

	// 计算相位
	double phase_rad = 2 * PI * F_t * t_Period + Phase;

	// 生成信号值
	SigOutput[0] = Amplitude * sin(phase_rad) + Offset;

	// 输出当前频率
	freqOutput[0] = F1_t;

	counter++;
	return true;
}