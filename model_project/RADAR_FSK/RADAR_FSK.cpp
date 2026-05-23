#include "RADAR_FSK.h"
#include <cmath>

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(RADAR_FSK)
{
	SET_MODEL_DESCRIPTION("FSK or FSK/PSK Coded Waveform Generator");
	SET_MODEL_SYMBOL("SYM_RADAR_FSK@RADAR Symbols");
	SET_MODEL_CATEGORY("Signal Source");

	ADD_MODEL_OUTPUT(output);
	//ADD_MODEL_OUTPUT(testVAR);
	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(Type, Types);
		enumParam.SetUnit(SystemVueModelBuilder::Units::NONE);
		enumParam.AddEnumeration("FSK", FSK); // 引号内显示名称
		enumParam.AddEnumeration("FSK_PSK", FSK_PSK);
		enumParam.SetDefaultValue("FSK");
		enumParam.SetDescription("FSK code type");
	}
	
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(PRI);
		param.SetUnit(SystemVueModelBuilder::Units::TIME);
		param.SetDefaultValue("1e-4");
		param.SetDescription("Pulse Repeat Interval");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(FHSequence);
		param.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		param.SetDefaultValue("[1e6,2e6,3e6]");
		param.SetDescription("Frequence Hopping Sequence");
		param.SetHideCondition("Type ~= 0");
	}
	
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(FSKPSKSequence);
		param.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		param.SetDefaultValue("[2,4,8,5,10,9,7,3,6,1]*1e5");
		param.SetDescription("FSK/PSK Sequence");
		param.SetHideCondition("Type ~= 1");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(TimeIntervals);
		param.SetUnit(SystemVueModelBuilder::Units::TIME);
		param.SetDefaultValue("[1e-5, 1e-5, 1e-5]");
		param.SetDescription("Frequence Hopping Time lntervals Sequence");
		param.SetHideCondition("Type ~= 0");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(FSKPSKSubTimePeriod);
		param.SetUnit(SystemVueModelBuilder::Units::TIME);
		param.SetDefaultValue("1e-5");
		param.SetDescription("FSK/PSK Subpulse duration period");
		param.SetHideCondition("Type ~= 1");
	}

	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(CodeLength, CodeLengthEnum);
		enumParam.SetUnit(SystemVueModelBuilder::Units::NONE);
		enumParam.AddEnumeration("Length_2_a", Length_2_a); // 引号内显示名称
		enumParam.AddEnumeration("Length_2_b", Length_2_b);
		enumParam.AddEnumeration("Length_3", Length_3);
		enumParam.AddEnumeration("Length_4_a", Length_4_a);
		enumParam.AddEnumeration("Length_4_b", Length_4_b);
		enumParam.AddEnumeration("Length_5", Length_5);
		enumParam.AddEnumeration("Length_7", Length_7);
		enumParam.AddEnumeration("Length_11", Length_11);
		enumParam.AddEnumeration("Length_13", Length_13);
		enumParam.SetDefaultValue("Length_13");
		enumParam.SetDescription("The Type of Barker Code");
		enumParam.SetHideCondition("Type ~= 1");
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

RADAR_FSK::RADAR_FSK()
{
	counter = 0;
}

//-----------------------------------------------------------------------------------
//	Setup
//		Declare the sample rate (if set) to SystemVue. If not set, the model will use the sample rate computed by SystemVue.
//-----------------------------------------------------------------------------------
bool RADAR_FSK::Setup()
{
	bool bStatus = true;
	if (SampleRate > 0)
	{
		// Use TimedCircularBuffer::SetSampleRate method to set the output sample rate
		//output.SetSampleRate(SampleRate);

		// 初始化巴克码序列
		if (barkerCodes.empty()) {
			InitializeBarkerCodes(barkerCodes);
		}
	}
	else
	{
		POST_ERROR("SampleRate must be greater than 0.");
		bStatus = false;
	}
	return bStatus;
}

//-----------------------------------------------------------------------------------
//	Initialize
//		Here we prepare a dataset of a pulsewidth for output in Run()
//-----------------------------------------------------------------------------------
bool RADAR_FSK::Initialize()
{   
	// 本函数准备一个脉宽内的数据点，供Run函数按节拍逐个发射
	bool bStatus = true;
	const double PI = 3.14159265358979323846;

	switch (Type){
	case FSK: // 该编码方式将一个脉冲分为多个频点进行调制，每个频点的持续时间可指定
	{
		// 脉内频点数、频点时间数必须相等
		if (FHSequence.NumElements() != TimeIntervals.NumElements()) {
			POST_ERROR("The size of FHSequence should be the same as TimeIntervals");
			return false;
		}

		// 脉内频点的持续时间必须小于等于PRI
		double pulseWidth = 0;
		for (int i = 0; i < TimeIntervals.NumElements(); ++i) {
			pulseWidth += TimeIntervals(i);
		}
		if (pulseWidth + 1e-17 > PRI) {
			POST_ERROR("sum of TimeIntervals should not larger than PRI");
			return false;
		}

		// 定义一个脉宽的信号数据
		int N_samples = static_cast<int>(pulseWidth * SampleRate); // 脉内总采样点数
		signal.resize(N_samples); // 配置信号缓存的大小

		int sampleIndex = 0;
		for (int i = 0; i < FHSequence.NumElements(); i++) {
			double subFreq = FHSequence(i);
			double timeInterval = TimeIntervals(i);
			int subFreqNumSamples = static_cast<int>(timeInterval * SampleRate);

			for (int j = 0; j < subFreqNumSamples; j++) {
				double t = j / SampleRate;
				double real = cos(2 * PI * subFreq * t);
				double imag = sin(2 * PI * subFreq * t);
				signal[sampleIndex] = std::complex<double>(real, imag);
				sampleIndex++;
			}
		}

		break;
	}
		
	case FSK_PSK: // 在FSK调制基础上，将调频子脉冲按巴克码序列进一步分解为多个子子脉冲进行相位调制
	{
		// 脉内频点的持续时间必须小于等于PRI
		double pulseWidth = FSKPSKSequence.NumElements() * FSKPSKSubTimePeriod;		
		if (pulseWidth + 1e-17 > PRI) {
			POST_ERROR("total time of FSKPSKSequence should not larger than PRI");
			return false;
		}
		
		// 定义一个脉宽的信号数据
		int N_samples = static_cast<int>(pulseWidth * SampleRate); // 脉内总采样点数
		signal.resize(N_samples); // 配置信号缓存的大小
		
		int sampleIndex = 0;

		// 先按每个频点准备子脉冲数据
		for (int i = 0; i < FSKPSKSequence.NumElements(); i++) {
			double freq = FSKPSKSequence(i);
			double subPulseDuration = FSKPSKSubTimePeriod;
			int subFreqNumSamples = static_cast<int>(subPulseDuration * SampleRate);

			// Get the corresponding Barker code for this sub-pulse
			std::vector<int> barkerCode = barkerCodes[CodeLength];

			// 再针对每个频点子脉冲，按巴克码数切分为子子脉冲
			int samplesPerBarkerElement = subFreqNumSamples / barkerCode.size();

			for (int j = 0; j < barkerCode.size(); j++) {
				int codeValue = barkerCode[j];
				for (int k = 0; k < samplesPerBarkerElement; k++) {
					// Correctly calculate the time variable t based on the current sample index
					double t = sampleIndex / static_cast<double>(SampleRate);
					double real = codeValue * cos(2 * PI * freq * t);
					double imag = codeValue * sin(2 * PI * freq * t);
					signal[sampleIndex] = std::complex<double>(real, imag);
					sampleIndex++;
				}
			}

			// 如果频点子脉冲不能整除巴克码数目，需补充空缺的频点子脉冲数据尾
			int pointGenerated = samplesPerBarkerElement * barkerCode.size();
			if (pointGenerated < subFreqNumSamples) {
				for (int k = pointGenerated; k < subFreqNumSamples; k++) {
					double t = sampleIndex / static_cast<double>(SampleRate);
					double real = cos(2 * PI * freq * t);
					double imag = sin(2 * PI * freq * t);
					signal[sampleIndex] = std::complex<double>(real, imag);
					sampleIndex++;
				}
			}
		}

		break;
	}
	}
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool RADAR_FSK::Run()
{
	bool bStatus = true;

	int PRI_PointNum = static_cast<int>(PRI * SampleRate);
	int currentPt = counter % PRI_PointNum;

	if (currentPt < signal.size()) {
		output[0].real(signal[currentPt].real());
		output[0].imag(signal[currentPt].imag());
	}
	else {
		output[0].real(0.0);
		output[0].imag(0.0);
	}

	counter++;

	return bStatus;
}