#include "Bits.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( Bits )
{	
	SET_MODEL_DESCRIPTION("Random Bit Generator");
	SET_MODEL_SYMBOL("SYM_Bits");
	SET_MODEL_CATEGORY("Sources");

	ADD_MODEL_OUTPUT(output); // bool, 黄色箭头

	// 参数：ProbOfZero
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(ProbOfZero);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("0.5");
		p.SetDescription("Probability of zero bit value");
		p.SetUseDefault(true);
		p.SetSchematicDisplay(0);
	}

	// 参数：BitRate
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(BitRate);
		p.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		p.SetDefaultValue("Sample_Rate");
		p.SetDescription("Output bit rate");
		p.SetUseDefault(true);
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
		e.SetUseDefault(true);
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
	}

	// 参数：InitialDelay
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(InitialDelay);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("0");
		p.SetDescription("Output sample delay");
		p.SetHideCondition("ShowAdvancedParams ~= 1");
		p.SetUseDefault(true);
		p.SetSchematicDisplay(0);
	}

	// 参数：BurstMode（枚举）
	{
		SystemVueModelBuilder::DFParam e =
			ADD_MODEL_ENUM_PARAM(BurstMode, BurstModeEnum);
		e.SetUnit(SystemVueModelBuilder::Units::NONE);
		e.AddEnumeration("OFF", OFF);
		e.AddEnumeration("Single", Single);
		e.AddEnumeration("Multiple", Multiple);
		e.SetDefaultValue("OFF");
		e.SetDescription("Burst mode");
		e.SetHideCondition("ShowAdvancedParams ~= 1");
		e.SetUseDefault(true);
		e.SetSchematicDisplay(0);
	}

	// 参数：BurstLength
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(BurstLength);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("100");
		p.SetDescription("Burst sample length");
		p.SetHideCondition("ShowAdvancedParams ~= 1 || BurstMode~= 1 && BurstMode~= 2");
		p.SetSchematicDisplay(0);
		p.SetUseDefault(true);
	}

	// 参数：BurstPeriod
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(BurstPeriod);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("200");
		p.SetDescription("Samples from start of one burst to start of next");
		p.SetHideCondition("ShowAdvancedParams ~= 1 || BurstMode~= 2");
		p.SetSchematicDisplay(0);
		p.SetUseDefault(true);
	}

	// 参数：BurstDelay
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(BurstDelay);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("0");
		p.SetDescription("Sample delay within burst before the start of the burst length interval");
		p.SetHideCondition("ShowAdvancedParams ~= 1 || BurstMode~= 1 && BurstMode~= 2");
		p.SetSchematicDisplay(0);
		p.SetUseDefault(true);
	}
	return true;
}
#endif

Bits::Bits()
{
	
}

bool Bits::Setup()
{
	bool bStatus = true;

	if (ProbOfZero < 0 || ProbOfZero > 1)
	{
		POST_ERROR("Probability of zero bit must in [0, 1]");
		bStatus = false;
	}

	if (BitRate < 0 || ProbOfZero > SampleRate)
	{
		POST_ERROR("Output bit rate must in [0, SampleRate]");
		bStatus = false;
	}

	if (SampleRate <= 0)
	{
		POST_ERROR("SampleRate must be > 0");
		bStatus = false;
	}

	if (InitialDelay < 0)
	{
		POST_ERROR("InitialDelay must be >= 0");
		bStatus = false;
	}

	if (BurstLength < 1)
	{
		POST_ERROR("BurstLength must be >= 1");
		bStatus = false;
	}

	if (BurstPeriod < 1)
	{
		POST_ERROR("BurstPeriod must be >= 1");
		bStatus = false;
	}

	if (BurstMode == 2 && (BurstDelay < 0 || BurstDelay > BurstPeriod - BurstLength))
	{
		POST_ERROR("BurstDelay must in [0, BurstPeriod-BurstLength]");
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
		POST_ERROR("InitialDelay must be positive.");
		bStatus = false;
	}

	return bStatus;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool Bits::Run()
{
	int i = GetCount(); // 当前采样点索引（从0开始）
	bool bitValue = false;
	bool isInValidInterval = false;
	int currentBurstStart = -1; // 当前有效区间起始点

	int samplesPerBit = SampleRate / BitRate;
	if (samplesPerBit <= 0) samplesPerBit = 1; // 避免除以零

	// --------------------------
	// 1. 计算有效区间及当前区间起始点currentBurstStart
	// --------------------------
	if (ShowAdvancedParams == NO)
	{
		isInValidInterval = true;
	}
	else
	{
		if (i < InitialDelay)
		{
			isInValidInterval = false;
		}
		else if (BurstMode == OFF)
		{
			isInValidInterval = true;
			currentBurstStart = InitialDelay;
		}
		else if (BurstMode == Single)
		{
			currentBurstStart = InitialDelay + BurstDelay;
			int burstEnd = currentBurstStart + BurstLength;
			isInValidInterval = (i >= currentBurstStart && i < burstEnd);
		}
		else // 周期性模式
		{
			int baseStart = InitialDelay + BurstDelay;
			// 计算当前周期的起始点（向下取整）
			int n = (i - baseStart) / BurstPeriod;
			if (n < 0) n = 0;
			currentBurstStart = baseStart + n * BurstPeriod;
			int burstEnd = currentBurstStart + BurstLength;
			isInValidInterval = (i >= currentBurstStart && i < burstEnd);
		}
	}

	// --------------------------
	// 2. 有效区间内生成bit（核心优化：以区间为基准计算bit周期）
	// --------------------------
	if (isInValidInterval)
	{
		// 计算相对于有效区间起始点的偏移量（关键！）
		int offset = i - currentBurstStart;
		// 判断是否为有效区间的第一个点（强制生成新bit）
		bool isBurstStartPoint = (offset == 0);
		// 判断是否为有效区间内的bit周期起始点（以区间为基准）
		bool isBitStartPoint = (offset % samplesPerBit == 0);

		// 要么是区间起点，要么是区间内的bit周期起点 → 生成新bit
		if (isBurstStartPoint || isBitStartPoint)
		{
			double randNum = static_cast<double>(rand()) / RAND_MAX;
			bitValue = (randNum >= ProbOfZero);
		}
		else
		{
			// 复用前一个bit（保证连续）
			bitValue = previousBitValue;
		}
		// 更新previousBitValue（仅有效区间内更新）
		previousBitValue = bitValue;
	}
	else
	{
		// 无效区间固定为false，不更新previousBitValue
		bitValue = false;
	}

	output[0] = bitValue;
	return true;
}