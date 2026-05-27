#include "RADAR_EWDeceptionJamming.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( RADAR_EWDeceptionJamming )
{	
	SET_MODEL_DESCRIPTION("EW Deception Jamming");

	SET_MODEL_CATEGORY("EW");

	ADD_MODEL_INPUT(signal);

	ADD_MODEL_OUTPUT(jamming);

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(SampleNum);
		param.SetDescription("The number of samples which this model generated each time when it is fired");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("1000");
	}
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(SampleRate);
		param.SetDescription("The SampleRate of the Jammer.");
		param.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		param.SetDefaultValue("10e6");
	}
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(FalseTargetNum);
		param.SetDescription("The False Target Number.");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("1");
	}
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(MaxRange);
		param.SetDescription("The Maximum Range in the simulation.");
		param.SetUnit(SystemVueModelBuilder::Units::LENGTH);
		param.SetDefaultValue("100e3");
	}
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(System_Loss);
		param.SetDescription("System and propogation loss in dB");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("0");
	}
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(FalseTargetRangeDelay);
		param.SetDescription("The range (delay) after the leading edge of real signal.");
		param.SetUnit(SystemVueModelBuilder::Units::LENGTH);
		param.SetDefaultValue("[100]");
	}
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(FalseTargetDopplerOffset);
		param.SetDescription("The doppler offset of real signal.");
		param.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		param.SetDefaultValue("[0]");
	}
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(FalseTargetGain);
		param.SetDescription("The gains of false targets.");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("[1]");
	}
	return true;
}
#endif

RADAR_EWDeceptionJamming::RADAR_EWDeceptionJamming()
{
	SampleIndex = 0;
}

bool RADAR_EWDeceptionJamming::Setup()
{
	bool bStatus = true;

	const double c = 3e8;
	// 参数校验
	if (SampleNum <= 0)
	{
		POST_ERROR("SampleNum must be > 0");
		bStatus = false;
	}
	if (SampleRate <= 0)
	{
		POST_ERROR("SampleRate must be > 0");
		bStatus = false;
	}
	if (FalseTargetNum <= 0)
	{
		POST_ERROR("FalseTargetNum must be > 0");
		bStatus = false;
	}
	if (MaxRange <= SampleNum / SampleRate * c / 2)
	{
		POST_ERROR("MaxRange must be > SampleNum / SampleRate * c / 2");
		bStatus = false;
	}
	if (FalseTargetRangeDelay.NumElements() != FalseTargetNum)
	{
		POST_ERROR("The size of FalseTargetRangeDelay must match the number of false targets(FalseTargetNum)");
		bStatus = false;
	}
	if (FalseTargetDopplerOffset.NumElements() != FalseTargetNum)
	{
		POST_ERROR("The size of FalseTargetRangeDelay must match the number of false targets(FalseTargetNum)");
		bStatus = false;
	}
	if (FalseTargetGain.NumElements() != FalseTargetNum)
	{
		POST_ERROR("The size of FalseTargetRangeDelay must match the number of false targets(FalseTargetNum)");
		bStatus = false;
	}
	signal.SetRate(SampleNum);
	jamming.SetRate(SampleNum);

	// 最大仿真点数
	MaxSampleNum = static_cast <int>(2 * MaxRange / c * SampleRate);
	// 干扰信号时延缓存器初始化
	FalseTargetDelayBuffer.Resize(1, MaxSampleNum);
	FalseTargetDelayBuffer.Zero();

	return bStatus;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool RADAR_EWDeceptionJamming::Run()
{
	const double c = 3e8;
	const double PI = std::acos(-1);
	const std::complex<double> imag_I(0, 1);

	for (int n = 0; n < FalseTargetNum; n++)
	{
		int NDelay = static_cast<int> (2 * FalseTargetRangeDelay(n) / c * SampleRate);

		// 将假目标干扰信号存入时延缓存器
		for (int i = 0; i < SampleNum; i++)
		{
			if (SampleIndex + i + NDelay < MaxSampleNum)
			{
				FalseTargetDelayBuffer(SampleIndex + i + NDelay) += signal[i] * std::exp(-imag_I * 2.0*PI*FalseTargetDopplerOffset(n))*FalseTargetGain(n);
			}		
		}
	}

	// 从干扰信号时延缓存器中输出各假目标的叠加信号
	for (int i = 0; i < SampleNum; i++)
	{
		jamming[i] += FalseTargetDelayBuffer(SampleIndex + i);
	}


	// 每个Run索引递增
	SampleIndex += SampleNum;
	// 干扰信号缓存满时重置缓存
	if (SampleIndex > MaxSampleNum)
	{
		FalseTargetDelayBuffer.Zero();
		SampleIndex = 0;
	}

	return true;
}
