#include "RADAR_TargetDetect.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( RADAR_TargetDetect )
{	
	SET_MODEL_DESCRIPTION("Target Detecting Algorithm which is used to detect target from noise");

	SET_MODEL_CATEGORY("Signal Processing");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(input);
		port.SetDescription("The input signal");
	}
	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(IsDetect);
		port.SetDescription("Whether target is detected or not");
	}
	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
		port.SetDescription("The output signal");
	}
	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(RangeBinIndex);
		port.SetDescription("The range bin index of detected target in the doppler dimension");
	}
	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(FreqBinIndex);
		port.SetDescription("The frequency bin index of detected target in the doppler dimension");
	}
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(PRI_Or_WaveGate);
		param.SetDescription("Time Gate to Collect Samples");
		param.SetUnit(SystemVueModelBuilder::Units::TIME);
		param.SetDefaultValue("10e-3");
	}
	//{
	//	SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(SampleNumForEstimateNoise);
	//	param.SetDescription("The number of noise samples");
	//	param.SetUnit(SystemVueModelBuilder::Units::NONE);
	//	param.SetDefaultValue("32");
	//}
	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(DetectType, SelectedDetectType);
		enumParam.SetDescription("the type of detecting: Range Dimension Detecting, 2D Detecing");
		enumParam.AddEnumeration("DetectRange", DetectRange);
		enumParam.AddEnumeration("Detect2D", Detect2D);
		enumParam.SetDefaultValue("0");
	}
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(FalseAlarmProbability);
		param.SetDescription("The probability of false alarm");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("1e-6");
	}
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(ReferenceCell);
		param.SetDescription("The number of samples/range bins which are regarded as Reference Cell");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("32");
	}
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(GuardCell);
		param.SetDescription("The number of samples/range bins which are regarded as Guard Cell");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("4");
	}
	//{
	//	SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(Coef1);
	//	param.SetDescription("The weight coefficient for noise samples in current wavegate for noise estimation");
	//	param.SetUnit(SystemVueModelBuilder::Units::NONE);
	//	param.SetDefaultValue("0.8");
	//	param.SetHideCondition("DetectType ~= 0");
	//}
	//{
	//	SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(Coef2);
	//	param.SetDescription("The weight coefficient for noise samples in last wavegate for noise estimation");
	//	param.SetUnit(SystemVueModelBuilder::Units::NONE);
	//	param.SetDefaultValue("0.2");
	//	param.SetHideCondition("DetectType ~= 0");
	//}
	//{
	//	SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(Coef);
	//	param.SetDescription("The weight coefficient for noise samples for 2-D noise estimation");
	//	param.SetUnit(SystemVueModelBuilder::Units::NONE);
	//	param.SetDefaultValue("1");
	//	param.SetHideCondition("DetectType ~= 1");
	//}
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(FreqChannelNum);
		param.SetDescription("The number of frequency channels when 2D target detecting is used");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("32");
		param.SetHideCondition("DetectType ~= 1");
	}
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(SampleRate);
		param.SetDescription("Waveform Baseband Sampling Rate");
		param.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		param.SetDefaultValue("10e6");
	}
	return true;
}
#endif

RADAR_TargetDetect::RADAR_TargetDetect()
{
	
}

bool RADAR_TargetDetect::Setup()
{
	bool bStatus = true;

	// 参数校验
	if (PRI_Or_WaveGate <= 0)
	{
		POST_ERROR("PRI_Or_WaveGate must be > 0");
		bStatus = false;
	}
	//if (SampleNumForEstimateNoise <= 0)
	//{
	//	POST_ERROR("SampleNumForEstimateNoise must be > 0");
	//	bStatus = false;
	//}
	if (FalseAlarmProbability <= 0 || FalseAlarmProbability > 1)
	{
		POST_ERROR("FalseAlarmProbability must be > 0 and <= 1");
		bStatus = false;
	}
	if (ReferenceCell <= 0)
	{
		POST_ERROR("ReferenceCell must be > 0");
		bStatus = false;
	}
	if (GuardCell <= 0)
	{
		POST_ERROR("GuardCell must be > 0");
		bStatus = false;
	}
	//if (Coef1 <= 0 || Coef2 <= 0 || Coef1 + Coef2 != 1)
	//{
	//	POST_ERROR("Coef1 and Coef2 must be: Coef1 > 0, Coef2 > 0, Coef1 + Coef2 = 1");
	//	bStatus = false;
	//}
	//if (Coef <= 0)
	//{
	//	POST_ERROR("Coef must be > 0");
	//	bStatus = false;
	//}
	//if (FreqChannelNum <= 0)
	//{
	//	POST_ERROR("FreqChannelNum must be > 0");
	//	bStatus = false;
	//}
	if (SampleRate <= 0)
	{
		POST_ERROR("SampleRate must be > 0");
		bStatus = false;
	}

	// 设置端口速率
	PRINum = PRI_Or_WaveGate * SampleRate;
	CellSize = DetectType ? PRINum * FreqChannelNum : PRINum;
	input.SetRate(CellSize);
	output.SetRate(CellSize);

	// 检测状态初始化
	DetectStatus = false;

	return bStatus;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool RADAR_TargetDetect::Run()
{
	// CFAR检测
	SystemVueModelBuilder::Matrix<double> LeadingWindow(1, ReferenceCell);
	SystemVueModelBuilder::Matrix<double> LaggingWindow(1, ReferenceCell);
	for (int i = 0; i < CellSize; i++)
	{
		// 获取当前参考窗（拼接法）
		for (int n = 0; n < ReferenceCell; n++)
		{
			int LeadingWindowIndex = i + n - GuardCell - ReferenceCell;
			int LaggingWindowIndex = i + n + GuardCell + 1;
			LeadingWindow(n) = std::abs(input[LeadingWindowIndex < 0 ? LeadingWindowIndex + CellSize : LeadingWindowIndex]);
			LaggingWindow(n) = std::abs(input[LaggingWindowIndex >= CellSize ? LaggingWindowIndex - CellSize : LaggingWindowIndex]);
		}

		double LeadingAvg = 0;
		double LaggingAvg = 0;
		for (int n = 0; n < ReferenceCell; n++)
		{
			LeadingAvg += LeadingWindow(n);
			LaggingAvg += LaggingWindow(n);
		}
		LeadingAvg /= ReferenceCell;
		LaggingAvg /= ReferenceCell;

		// CFAR门限因子计算
		double ThresholdFactor = 2 * ReferenceCell*(pow(FalseAlarmProbability, -1.0 / (2 * ReferenceCell)) - 1);

		// CFAR门限(CA-CFAR)
		Threshold = ThresholdFactor * (LeadingAvg + LaggingAvg) / 2;

		// 门限比较输出
		if (std::abs(input[i]) > Threshold)
		{
			output[i] = input[i];
			DetectStatus = true;
		}
		else
		{
			output[i] = 0;
		}
	}
	
	// 若检出目标，则输出目标的距离维与速度维索引
	if (DetectStatus)
	{
		// 快时间维求距离量
		double maxValue = 0.0;
		int maxIndex = 0;
		for (int i = 0; i < CellSize; i++)
		{
			if (std::abs(input[i]) > maxValue)
			{
				maxValue = std::abs(input[i]);
				maxIndex = i;
			}
		}
		// 距离维输出
		RangeBinIndex[0] = fmod(maxIndex, PRINum);

		// 慢时间维求速度量
		int fmaxIndex = std::floor(maxIndex / PRINum);
		// 速度维输出
		FreqBinIndex[0] = fmaxIndex;
	}
	else
	{
		RangeBinIndex[0] = 0;
		FreqBinIndex[0] = 0;
	}

	// 输出检测状态
	IsDetect[0] = DetectStatus;
	// 每个Run重置检测状态
	DetectStatus = false;

	return true;
}
