#include "RADAR_FSK.h"
#include <cmath>
#include <algorithm>

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
		enumParam.AddEnumeration("FSK", FSK); // 添加枚举显示项
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
		enumParam.AddEnumeration("Length_2_a", Length_2_a); // 添加枚举显示项
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

		// 初始化Barker码表
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
	// 准备一个脉冲周期内的数据点，供Run()循环输出
	bool bStatus = true;
	const double PI = 3.14159265358979323846;

	switch (Type){
	case FSK: // 跳频编码方式：将一段脉冲分解为多个跳频点，每个跳频点的持续时间由时间间隔指定
	{
		// 校验跳频序列和时间间隔的元素个数是否一致
		if (FHSequence.NumElements() != TimeIntervals.NumElements()) {
			POST_ERROR("The size of FHSequence should be the same as TimeIntervals");
			return false;
		}

		// 跳频序列的总持续时间不应超过PRI
		double pulseWidth = 0;
		for (int i = 0; i < TimeIntervals.NumElements(); ++i) {
			pulseWidth += TimeIntervals(i);
		}
		if (pulseWidth + 1e-17 > PRI) {
			POST_ERROR("sum of TimeIntervals should not larger than PRI");
			return false;
		}

		// 生成一个周期的基带信号
		int N_samples = static_cast<int>(pulseWidth * SampleRate); // 计算总采样点数
		signal.resize(N_samples); // 调整信号缓冲区大小

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
		
	case FSK_PSK: // 在FSK编码基础上，叠加了Barker码调制
	{
		// 确保Barker码表已初始化（Initialize在Setup之前调用）
		if (barkerCodes.empty()) {
			InitializeBarkerCodes(barkerCodes);
		}

		// 校验总脉冲宽度不超过PRI
		double pulseWidth = FSKPSKSequence.NumElements() * FSKPSKSubTimePeriod;
		if (pulseWidth + 1e-17 > PRI) {
			POST_ERROR("total time of FSKPSKSequence should not larger than PRI");
			return false;
		}

		// 计算每个子脉冲的采样点数
		int subFreqNumSamples = static_cast<int>(std::round(FSKPSKSubTimePeriod * SampleRate));
		int numElements = FSKPSKSequence.NumElements();
		// 总采样点数 = 子脉冲数 × 每个子脉冲的采样点（避免浮点精度导致的不一致）
		int N_samples = numElements * subFreqNumSamples;
		signal.resize(N_samples); // 调整信号缓冲区大小
		
		int sampleIndex = 0;

		// 逐个频率点进行Barker码调制
		for (int i = 0; i < numElements; i++) {
			double freq = FSKPSKSequence(i);

			// Get the corresponding Barker code for this sub-pulse
			std::vector<int> barkerCode = barkerCodes[CodeLength];

			// 将每个频率段的采样点按Barker码元素进行相位编码
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

			// 若频率段采样点不足Barker码整周期，则补充未调制的载波尾部
			int pointGenerated = samplesPerBarkerElement * barkerCode.size();
			if (pointGenerated < subFreqNumSamples) {
				for (int k = pointGenerated; k < subFreqNumSamples; k++) {
					if (sampleIndex >= N_samples) break; // 防止越界
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
	return bStatus;
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