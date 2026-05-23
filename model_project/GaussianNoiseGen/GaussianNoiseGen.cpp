#include "GaussianNoiseGen.h"
#include <random>

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( GaussianNoiseGen )
{	
	SET_MODEL_DESCRIPTION("Gaussian Noise Generator");
	SET_MODEL_SYMBOL("SYM_IID_Gaussian");
	SET_MODEL_CATEGORY("Sources");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(NDensity);
		param.SetDescription("Noise power spectral density");
		param.SetUnit(SystemVueModelBuilder::Units::POWER);
		param.SetDefaultValue("4.00388587e-21");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(RefR);
		param.SetDescription("Reference resistance");
		param.SetUnit(SystemVueModelBuilder::Units::RESISTANCE);
		param.SetDefaultValue("50");
		param.SetSchematicDisplay(0);
		param.SetUseDefault(1);
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

	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(BurstMode, SelectedBurstMode);
		enumParam.SetDescription("Burst mode: OFF, Single, Multiple");
		enumParam.AddEnumeration("OFF", OFF);
		enumParam.AddEnumeration("Single", Single);
		enumParam.AddEnumeration("Multiple", Multiple);
		enumParam.SetHideCondition("ShowAdvancedParams ~= 1");
		enumParam.SetDefaultValue("0");
		enumParam.SetSchematicDisplay(0);
		enumParam.SetUseDefault(1);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(BurstLength);
		param.SetDescription("Burst sample length");
		param.SetDefaultValue("100");
		param.SetHideCondition("ShowAdvancedParams ~= 1 || BurstMode == 0");
		param.SetSchematicDisplay(0);
		param.SetUseDefault(1);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(BurstPeriod);
		param.SetDescription("Samples from start of one burst to start of next");
		param.SetDefaultValue("200");
		param.SetHideCondition("ShowAdvancedParams ~= 1 || BurstMode ~= 2");
		param.SetSchematicDisplay(0);
		param.SetUseDefault(1);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(BurstDelay);
		param.SetDescription("Sample delay within burst before the start of the burst length interval");
		param.SetDefaultValue("0");
		param.SetHideCondition("ShowAdvancedParams ~= 1 || BurstMode == 0");
		param.SetSchematicDisplay(0);
		param.SetUseDefault(1);
	}
	return true;
}
#endif

GaussianNoiseGen::GaussianNoiseGen()
{
	
}

bool GaussianNoiseGen::Setup()
{
	bool bStatus = true;

	if (InitialDelay < 0)
	{
		POST_ERROR("InitialDelay must not be negtive.");
		bStatus = false;
	}

	if (BurstLength < 1)
	{
		POST_ERROR("BurstLength must be greater than 0.");
		bStatus = false;
	}

	if (BurstPeriod < 1)
	{
		POST_ERROR("BurstPeriod must be greater than 0.");
		bStatus = false;
	}

	if (BurstDelay < 0 || BurstDelay > BurstPeriod - BurstLength)
	{
		POST_ERROR("BurstDelay must be greater than 0 and less than BurstPeriod - BurstLength");
		bStatus = false;
	}

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

	return bStatus;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool GaussianNoiseGen::Run()
{
	int i = GetCount();

	SampleRate = output.GetSampleRate();
	double StdDev = std::sqrt(NDensity*(SampleRate / 2)*RefR);

	// 生成高斯噪声
	std::random_device rd;	// 随机器
	std::mt19937 gen(rd()); // 梅森旋转生成种子

	std::normal_distribution<double>	dN(0, StdDev);
	output[0] = dN(gen);

	// 初始时延
	if (i < InitialDelay)
	{
		output[0] = 0.0;
	}

	// 单脉冲模式输出
	if (BurstMode == GaussianNoiseGen::Single)
	{
		if (i < InitialDelay + BurstDelay || i >= InitialDelay + BurstDelay + BurstLength)
		{
			output[0] = 0.0;
		}
	}

	// 多脉冲模式疏输出
	if (BurstMode == GaussianNoiseGen::Multiple)
	{
		int wi = (i - InitialDelay) % BurstPeriod; // 当前索引对应窗内索引

		if (wi < BurstDelay || wi >= BurstDelay + BurstLength)
		{
			output[0] = 0.0;
		}
	}

	return true;
}
