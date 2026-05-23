#include "RADAR_EchoGenerator.h"



#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( RADAR_EchoGenerator )
{	
	SET_MODEL_DESCRIPTION("To generate the echo signals");

	SET_MODEL_CATEGORY("Environments");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(inSignal);
		port.SetDescription("The transmit radar waveforms from different radar transmitters");
	}
	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(TxPlatformLoc);
		port.SetDescription("The transmit radar platform trajectory");
	}
	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(RxPlatformLoc);
		port.SetDescription("The receive radar platform trajectory");
		//port.SetOptional();
	}
	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(TargetScatterLoc);
		port.SetDescription("The target trajectory");
		//port.SetOptional();
	}
	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(TargetScatterRCS);
		port.SetDescription("The RCS of target scatters");
		//port.SetOptional();
	}
	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(TargetSignal);
		port.SetDescription("The composite radar signals which will be received by targets");
	}
	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(outSignal);
		port.SetDescription("The composite radar echoes which will be received by different radar receivers");
	}
	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(RxSignal);
		port.SetDescription("The composite radar signals which will be directly received by different radar receivers");
	}
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(SampleRate);
		param.SetDescription("The sampling rate");
		param.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		param.SetDefaultValue("10e6");
	}
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(SystemLoss);
		param.SetDescription("System loss (Combined loss of Feed Network, Feedline, Antenna Radome, etc.) in dB");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("0");
	}
	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(IncludePropagationEffect, SelectedIncludePropagationEffect);
		enumParam.AddEnumeration("No", No);
		enumParam.AddEnumeration("Yes", Yes);
		enumParam.SetDefaultValue("1");
	}
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(RF_Freq);
		param.SetDescription("Carrier frequency");
		param.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		param.SetDefaultValue("10e9");
		param.SetHideCondition("IncludePropagationEffect ~= 1");
	}
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(SimulationSampleNum);
		param.SetDescription("Max num of simulation samples");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("1000000");
		param.SetSchematicDisplay(0);
	}
	return true;
}
#endif

RADAR_EchoGenerator::RADAR_EchoGenerator()
{
	Index = 0;
}

bool RADAR_EchoGenerator::Setup()
{
	bool bStatus = true;

	//  参数校验
	if (SampleRate <= 0)
	{
		POST_ERROR("SampleRate must be > 0");
		bStatus = false;
	}
	if (SimulationSampleNum <= 0)
	{
		POST_ERROR("SimulationSampleNum must be > 0");
		bStatus = false;
	}
	if (RF_Freq <= 0)
	{
		POST_ERROR("RF_Freq must be > 0");
		bStatus = false;
	}
	if (TargetScatterLoc.GetSize() == TargetScatterRCS.GetSize())
	{
		TargetNum = TargetScatterRCS.GetSize();	// 目标数量
	}
	else
	{
		POST_ERROR("Port size of TargetScatterLoc and TargetScatterRCS must be the same");
		bStatus = false;
	}

	TxPlatformNum = TxPlatformLoc.GetSize();	// 雷达发射平台数量
	RxPlatformNum = RxPlatformLoc.GetSize();	// 雷达接收平台数量
	ChannelNum = inSignal.GetSize();			// 信号通道数量（雷达阵元数量）

	if (ChannelNum)
	{
		TargetDelayBuffer.Resize(ChannelNum, SimulationSampleNum);
		outDelayBuffer.Resize(ChannelNum, SimulationSampleNum);
		RxDelayBuffer.Resize(ChannelNum, SimulationSampleNum);
		TargetDelayBuffer.Zero();
		outDelayBuffer.Zero();
		RxDelayBuffer.Zero();
	}
	return bStatus;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool RADAR_EchoGenerator::Run()
{
	//const double c = 3e8;
	const double c = 299792458;
	const double pi = std::acos(-1);
	const std::complex<double> imag_i{ 0,1 };

	SystemVueModelBuilder::Matrix<double>	TargetX(1, TargetNum), TargetY(1, TargetNum), TargetZ(1, TargetNum);	// 声明各目标的绝对坐标
	SystemVueModelBuilder::Matrix<double>	TxX(1, TxPlatformNum), TxY(1, TxPlatformNum), TxZ(1, TxPlatformNum);	// 声明各雷达发射平台的绝对坐标
	SystemVueModelBuilder::Matrix<double>	RxX(1, RxPlatformNum), RxY(1, RxPlatformNum), RxZ(1, RxPlatformNum);	// 声明各雷达接收平台的绝对坐标

	for (int i = 0; i < TargetNum; i++)
	{
		// 计算各目标在 ECI 坐标系下的坐标
		TargetX(i) = TargetScatterLoc[i][0](0);
		TargetY(i) = TargetScatterLoc[i][0](1);
		TargetZ(i) = TargetScatterLoc[i][0](2);
	}

	for (int m = 0; m < TxPlatformNum; m++)
	{
		// 计算各雷达发射平台在 ECI 坐标系下的坐标
		TxX(m) = TxPlatformLoc[m][0](0);
		TxY(m) = TxPlatformLoc[m][0](1);
		TxZ(m) = TxPlatformLoc[m][0](2);
	}

	for (int n = 0; n < RxPlatformNum; n++)
	{
		// 计算各雷达接收平台在 ECI 坐标系下的坐标
        RxX(n) = RxPlatformLoc[n][0](0);
        RxY(n) = RxPlatformLoc[n][0](1);
        RxZ(n) = RxPlatformLoc[n][0](2);
	}

	// 发射平台至目标传播路径下的路径长度、时延、衰减
	for (int m = 0; m < TxPlatformNum; m++)
	{
		for (int i = 0; i < TargetNum; i++)
		{
			// 发射平台至目标的距离
			double TargetRange = std::sqrt((TxX(m) - TargetX(i))*(TxX(m) - TargetX(i)) + (TxY(m) - TargetY(i))*(TxY(m) - TargetY(i)) + (TxZ(m) - TargetZ(i))*(TxZ(m) - TargetZ(i)));
			// 发射平台至目标的时延
			double TargetDelay = TargetRange / c;
			int TargetDelayN = static_cast<int>(TargetDelay * SampleRate);
			// 传播效应（衰减、相移）
			double TargetAtten = 1;
			double TargetPhaseShift = 0;
			if (IncludePropagationEffect)
			{
				// 发射平台至目标的衰减（相对值），注意作用在信号幅值上需要开方
				TargetAtten = TargetRange ? 1 / std::sqrt(8 * pi * std::sqrt(pi) * TargetRange * TargetRange / (c / RF_Freq)) : 1;
				// 相移
				TargetPhaseShift = 2 * pi*RF_Freq*TargetDelay;
			}
			// 多径合成
			for (int k = 0; k < ChannelNum; k++)
			{
				if (Index + TargetDelayN < SimulationSampleNum)
				{
					TargetDelayBuffer(k, Index + TargetDelayN) += inSignal[k][0].complex() * TargetAtten * std::exp(-imag_i * TargetPhaseShift);
				}
			}
		}
	}

	// 目标至接收平台传播路径下的路径长度、时延、衰减
	for (int i = 0; i < TargetNum; i++)
	{
		for (int n = 0; n < RxPlatformNum; n++)
		{
			// 目标至接收平台的距离
			double outRange = std::sqrt((RxX(n) - TargetX(i))*(RxX(n) - TargetX(i)) + (RxY(n) - TargetY(i))*(RxY(n) - TargetY(i)) + (RxZ(n) - TargetZ(i))*(RxZ(n) - TargetZ(i)));
			// 目标至接收平台的时延
			double outDelay = outRange / c;
			int outDelayN = static_cast<int>(outDelay * SampleRate);
			// 传播效应（衰减、相移）
			double outAtten = 1;
			double outPhaseShift = 0;
			if (IncludePropagationEffect)
			{
				// 目标至接收平台的衰减（相对值），注意作用在信号幅值上需要开方
				outAtten = outRange ? 1 / std::sqrt(8 * pi * std::sqrt(pi) * outRange * outRange / (c / RF_Freq)) : 1;
				// 相移
				outPhaseShift = 2 * pi*RF_Freq*outDelay;
			}
			// 多径合成
			for (int k = 0; k < ChannelNum; k++)
			{
				if (Index + outDelayN < SimulationSampleNum)
				{
					outDelayBuffer(k, Index + outDelayN) += TargetDelayBuffer(k, Index) * outAtten * std::exp(-imag_i * outPhaseShift) * TargetScatterRCS[k][0];
				}
			}
		}
	}

	// 发射平台至接收平台（直达信号）的路径长度、时延、衰减
	for (int m = 0; m < TxPlatformNum; m++)
	{
		for (int n = 0; n < RxPlatformNum; n++)
		{
			// 发射平台至接收平台的距离
			double RxRange = std::sqrt((TxX(m) - RxX(n))*(TxX(m) - RxX(n)) + (TxY(m) - RxY(n))*(TxY(m) - RxY(n)) + (TxZ(m) - RxZ(n))*(TxZ(m) - RxZ(n)));
			// 发射平台至接收平台的时延
			double RxDelay = RxRange / c;
			int RxDelayN = static_cast<int>(RxDelay * SampleRate);
			// 传播效应（衰减、相移）
			double RxAtten = 1;
			double RxPhaseShift = 0;
			if (IncludePropagationEffect)
			{
				// 发射平台至接收平台的衰减（相对值），注意作用在信号幅值上需要开方
				RxAtten = RxRange ? 1 / std::sqrt(8 * pi * std::sqrt(pi) * RxRange * RxRange / (c / RF_Freq)) : 1;
				// 相移
				RxPhaseShift = 2 * pi*RF_Freq*RxDelay;
			}
			// 多径合成
			for (int k = 0; k < ChannelNum; k++)
			{
				if (Index + RxDelayN < SimulationSampleNum)
				{
					RxDelayBuffer(k, Index + RxDelayN) += inSignal[k][0].complex() * RxAtten * std::exp(-imag_i * RxPhaseShift);
				}
			}
		}
	}

	// 输出当前索引下的缓存内容
	for (int k = 0; k < ChannelNum; k++)
	{
		if (k < TargetSignal.GetSize())
		{
			TargetSignal[k][0] = TargetDelayBuffer(k, Index);
		}
		if (k < outSignal.GetSize())
		{
			outSignal[k][0] = outDelayBuffer(k, Index) * std::pow(10, -SystemLoss / 20);
		}
		if (k < RxSignal.GetSize())
		{
			RxSignal[k][0] = RxDelayBuffer(k, Index);
		}
	}

	// 每个Run索引位置递增
	Index++;

	return true;
}
