#include "RADAR_PULSE.h"
#include <cmath>

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(RADAR_PULSE)
{
	SET_MODEL_DESCRIPTION("Simple Pulse Waveform Generator");
	SET_MODEL_SYMBOL("SYM_RADAR_PULSE@RADAR Symbols");
	SET_MODEL_CATEGORY("Signal Source");

	ADD_MODEL_OUTPUT(output);
	//ADD_MODEL_OUTPUT(testVAR);

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(Pulsewidth);
		param.SetUnit(SystemVueModelBuilder::Units::TIME);
		param.SetDefaultValue("[1e-5]");
		param.SetDescription("Pulse Width");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(PRI);
		param.SetUnit(SystemVueModelBuilder::Units::TIME);
		param.SetDefaultValue("[1e-4]");
		param.SetDescription("Pulse Repeat Interval");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(PRI_Combination);
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("[1]");
		param.SetDescription("PRl Combination reprents by each PRl repeat number");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(SampleRate);
		param.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		param.SetDefaultValue("10e6");
		param.SetDescription("Waveform Baseband Sampling Rate");
	}
	return true;
}
#endif

RADAR_PULSE::RADAR_PULSE()
{
	counter = 0;
}

//-----------------------------------------------------------------------------------
//	Setup
//		Declare the sample rate (if set) to SystemVue. If not set, the model will use the sample rate computed by SystemVue.
//-----------------------------------------------------------------------------------
bool RADAR_PULSE::Setup()
{
	bool bStatus = true;
	if (SampleRate > 0)
	{
		// Use TimedCircularBuffer::SetSampleRate method to set the output sample rate
		// output.SetSampleRate(SampleRate);
	}
	else
	{
		POST_ERROR("SampleRate must be greater than 0.");
		bStatus = false;
	}
	return bStatus;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool RADAR_PULSE::Run()
{
	bool bStatus = true;
	//Use TimedCircularBuffer::GetTime method to get the time stamp of the output sample
	//In output.GetTime( 0, m_iFiringCount ), 0 means the 0th output sample of each firing (run), and TimedDFModel::GetCount returns the current firing count.

	const double PI = 3.14159265358979323846;

	double t = counter / SampleRate;//output.GetTime(0, m_iFiringCount);

	// 计算一组PRI的总时间（如 [1,2,1] 对应 PRI1 + 2 * PRI2 + PRI3）
	double groupTime = 0.0;
	for (int i = 0; i < PRI_Combination.NumElements(); i++) {
		groupTime += PRI_Combination(i) *  PRI(i);
	}

	// 确定当前脉冲的索引
	int pulseIndex = 0;
	double insideT = fmodl(t, groupTime - 1.0e-16); // 一组脉冲内的计时器
	double accumulatedTime = 0.0;

	for (int i = 0; i < PRI_Combination.NumElements(); i++)
	{
		double pulseDuration = PRI_Combination(i) *  PRI(i);
		if (insideT < accumulatedTime + pulseDuration) {
			pulseIndex = i;
			break;
		}
		accumulatedTime += pulseDuration;
	}

	// 获取当前脉冲的参数
	double currentPRI = PRI(pulseIndex);
	double currentPW = Pulsewidth(pulseIndex);

	// 生成每个PRI内的脉冲信号
	double pulseOffsetTime = insideT - accumulatedTime;			// 计算当前脉冲在当前脉冲组内的时间
	double localTime = fmodl(pulseOffsetTime, currentPRI - 1.0e-16);	// 计算当前脉冲的内部时间

	if (localTime >= 0 && localTime < currentPW - 1.0e-16) {
		output[0] = 1.0;
	}
	else
	{
		output[0] = 0.0;

	}

	counter++;

	//testVAR = std::ceill(localTime*1e16)- localTime * 1e16; // 测试输出
	return bStatus;
}