#include "RADAR_BarkerCode.h"
#include <cmath>

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(RADAR_BarkerCode)
{
	SET_MODEL_DESCRIPTION("Barker Coded Waveform Generator");
	SET_MODEL_SYMBOL("SYM_RADAR_BarkerCode@RADAR Symbols");
	SET_MODEL_CATEGORY("Signal Source");

	ADD_MODEL_OUTPUT(output);
	//ADD_MODEL_OUTPUT(testVAR);
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(PRI);
		param.SetUnit(SystemVueModelBuilder::Units::TIME);
		param.SetDefaultValue("1e-4");
		param.SetDescription("Pulse Repeat Interval");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(SubPulseWidth);
		param.SetUnit(SystemVueModelBuilder::Units::TIME);
		param.SetDefaultValue("1e-6");
		param.SetDescription("SubPulse or Each Code Width");
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

RADAR_BarkerCode::RADAR_BarkerCode()
{
	counter = 0;
}

//-----------------------------------------------------------------------------------
//	Setup
//		Declare the sample rate (if set) to SystemVue. If not set, the model will use the sample rate computed by SystemVue.
//-----------------------------------------------------------------------------------
bool RADAR_BarkerCode::Setup()
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
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool RADAR_BarkerCode::Run()
{
	bool bStatus = true;

	// 获取当前巴克码序列
	const std::vector<int>& code = barkerCodes[CodeLength];

	// 计算每个子脉冲和 PRI 的采样点数
	int samplesPerSubPulse = static_cast<int>(SubPulseWidth * SampleRate);
	int samplesPerCode = static_cast<int>(code.size() * samplesPerSubPulse);
	int samplesPerPRI = static_cast<int>(PRI * SampleRate);

	// 计算当前子脉冲和码元索引
	int subPulseIndex = counter / samplesPerSubPulse;
	int codeIndex = subPulseIndex % code.size();

	// 生成输出信号
	if (counter < samplesPerCode) {
		output[0] = std::complex<double>(code[codeIndex], 0.0); // 巴克码信号
	}
	else {
		output[0] = std::complex<double>(0.0, 0.0); // PRI 剩余部分填充 0
	}

	// 更新采样点索引
	counter = (counter + 1) % samplesPerPRI;

	//testVAR = code[codeIndex]; // 测试输出
	return bStatus;
}