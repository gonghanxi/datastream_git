#include "RADAR_TargetTrack.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( RADAR_TargetTrack )
{	
	SET_MODEL_DESCRIPTION("Target Tracking Algorithm which is used to track single target");

	SET_MODEL_CATEGORY("Signal Processing");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(isTrack);
		port.SetDescription("Whether the target is tracked or not");
	}
	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(input);
		port.SetDescription("The input signal");
	}
	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
		port.SetDescription("The output signal");
	}
	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(Range);
		port.SetDescription("The measured range");
	}
	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(GateStart);
		port.SetDescription("The start position of wave gate, which will fed to wave gate model");
	}
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(PRI_Or_WaveGate);
		param.SetDescription("Time Gate to Collect Samples");
		param.SetUnit(SystemVueModelBuilder::Units::TIME);
		param.SetDefaultValue("10e-6");
	}
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(TrackGate);
		param.SetDescription("Time Gate to Track the Target");
		param.SetUnit(SystemVueModelBuilder::Units::TIME);
		param.SetDefaultValue("10e-6");
	}
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(InitGateStartTime);
		param.SetDescription("Initial WaveGate Start Time which is used to search target");
		param.SetUnit(SystemVueModelBuilder::Units::TIME);
		param.SetDefaultValue("10e-6");
	}
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(SampleRate);
		param.SetDescription("Time Gate to Collect Samples");
		param.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		param.SetDefaultValue("10e6");
	}
	return true;
}
#endif

RADAR_TargetTrack::RADAR_TargetTrack()
{

}

bool RADAR_TargetTrack::Setup()
{
	bool bStatus = true;

	// 参数校验
	if (PRI_Or_WaveGate <= 0)
	{
		POST_ERROR("PRI_Or_WaveGate must be > 0");
		bStatus = false;
	}
	if (TrackGate <= 0 || TrackGate > PRI_Or_WaveGate)
	{
		POST_ERROR("TrackGate must be > 0 and <= PRI_Or_WaveGate");
		bStatus = false;
	}
	//if (InitGateStartTime <= 0)
	//{
	//	POST_ERROR("InitGateStartTime must be > 0");
	//	bStatus = false;
	//}
	if (SampleRate <= 0)
	{
		POST_ERROR("SampleRate must be > 0");
		bStatus = false;
	}

	// 设置端口速率
	PRINum = PRI_Or_WaveGate * SampleRate;
	input.SetRate(PRINum);
	output.SetRate(PRINum);

	// 门限控制时间初始化
	GateStartTime = InitGateStartTime;

	return bStatus;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool RADAR_TargetTrack::Run()
{
	if (isTrack[0])
	{
		// 快时间维求距离量
		double maxValue = 0.0;
		int maxIndex = 0;
		for (int i = 0; i < PRINum; i++)
		{
			if (std::abs(input[i]) > maxValue)
			{
				maxValue = std::abs(input[i]);
				maxIndex = i;
			}
		}
		// 距离维输出
		maxIndex = fmod(maxIndex, PRINum);
		const double c = 3e8;
		Range[0] = c * ((maxIndex / SampleRate) + GateStartTime) / 2;

		// 门限控制时间根据目标与门限中心的偏移量变化，反馈给波门模型以使得目标尽可能落入门限中心，实现跟踪的目的
		GateStartTime += maxIndex / SampleRate - PRI_Or_WaveGate / 2;
		GateStart[0] = GateStartTime;

		// 信号输出
		for (int i = 0; i < PRINum; i++)
		{
			output[i] = input[i];
		}
	}
	else
	{
		// 距离维输出
		Range[0] = 0;

		// 门限控制时间输出
		GateStart[0] = InitGateStartTime;

		// 信号输出
		for (int i = 0; i < PRINum; i++)
		{
			output[i] = 0;
		}
	}
	return true;
}
