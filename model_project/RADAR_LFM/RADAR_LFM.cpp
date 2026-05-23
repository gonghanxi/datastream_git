#include "RADAR_LFM.h"
#include <cmath>

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(RADAR_LFM)
{
	SET_MODEL_DESCRIPTION("Linear Frequency Modulation Waveform Generator");
	SET_MODEL_SYMBOL("SYM_RADAR_LFM@RADAR Symbols");
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
		param.SetDescription("PRI Combination represents the number of repetition for each PRI");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(Bandwidth);
		param.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		param.SetDefaultValue("[5e6]");
		param.SetDescription("Waveform Bandwidth");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(FM_Offset);
		param.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		param.SetDefaultValue("[0]");
		param.SetDescription("Frequency Modulation Offset");
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

RADAR_LFM::RADAR_LFM()
{
	counter = 0;
}

//-----------------------------------------------------------------------------------
//	Setup
//		Declare the sample rate (if set) to SystemVue. If not set, the model will use the sample rate computed by SystemVue.
//-----------------------------------------------------------------------------------
bool RADAR_LFM::Setup()
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
bool RADAR_LFM::Run()
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
	double currentBW = Bandwidth(pulseIndex);
	double currentOF = FM_Offset(pulseIndex);

	// 计算每个PRI内的线性调频信号
	double pulseOffsetTime = insideT - accumulatedTime;			// 计算当前脉冲在当前脉冲组内的时间
	double localTime = fmodl(pulseOffsetTime, currentPRI - 1.0e-16);	// 计算当前脉冲的局部时间

	if (localTime >= 0 && localTime < currentPW - 1.0e-16) {
		double chirpRate = currentBW / currentPW;						// 调频斜率
		double tau = localTime;											// 脉冲内时间

		output[0].real(cos(PI * chirpRate * (tau - currentPW / 2.0) * (tau - currentPW / 2.0) + 2.0 * PI * currentOF * tau));
		output[0].imag(sin(PI * chirpRate * (tau - currentPW / 2.0) * (tau - currentPW / 2.0) + 2.0 * PI * currentOF * tau));
	}
	else
	{
		output[0].real(0.0);
		output[0].imag(0.0);
	}

	counter++;

	//testVAR = std::ceill(localTime*1e16)- localTime * 1e16; // 测试输出
	return bStatus;
}