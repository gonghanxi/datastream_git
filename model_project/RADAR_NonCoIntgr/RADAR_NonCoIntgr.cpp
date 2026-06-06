#include "RADAR_NonCoIntgr.h"

#ifndef SV_CODE_GEN

DEFINE_MODEL_INTERFACE(RADAR_NonCoIntgr)
{
	// ===== 模型基本信息 =====
	SET_MODEL_DESCRIPTION("Signal non-coherent Integration");
	SET_MODEL_SYMBOL("SYM_RADAR_NonCoIntgr@RADAR Symbols");
	SET_MODEL_CATEGORY("Signal Processing");

	// ===== 端口 =====
	ADD_MODEL_INPUT(input);
	ADD_MODEL_OUTPUT(output);

	// ===== 参数：PRI_Or_WaveGate =====
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(PRI_Or_WaveGate);
		param.SetDescription("Time Gate to Collect Samples");
		param.SetUnit(SystemVueModelBuilder::Units::TIME);
		param.SetDefaultValue("10e-3");
	}

	// ===== 参数：Number =====
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(Number);
		param.SetDescription("Number of Pulses for non-coherent integration");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("5");
	}

	// ===== 参数：SampleRate =====
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(SampleRate);
		param.SetDescription("Waveform Baseband Sampling Rate");
		param.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		param.SetDefaultValue("10e6");
	}

	return true;
}

#endif


//------------------------------------------------------------------------------
// 构造函数
//------------------------------------------------------------------------------
RADAR_NonCoIntgr::RADAR_NonCoIntgr()
	: PRI_Or_WaveGate(10e-3)
	, Number(5)
	, SampleRate(10e6)
	, samplesPerPulse_(0)
	, inputRate_(0)
	, outputRate_(0)
{
}


//------------------------------------------------------------------------------
// Setup
// 根据帮助文档：
// input  Rate = PRI_Or_WaveGate * SampleRate * Number
// output Rate = PRI_Or_WaveGate * SampleRate
//------------------------------------------------------------------------------
bool RADAR_NonCoIntgr::Setup()
{
	if (PRI_Or_WaveGate <= 0.0 || SampleRate <= 0.0 || Number <= 0)
	{
		return false;
	}

	// 每个脉冲 / WaveGate 内采样点数
	// 这里使用 round，是因为内置帮助文档给的是时间 * 采样率，
	// 黑盒测试中也表现为整数采样点分组。
	samplesPerPulse_ = static_cast<int>(std::round(PRI_Or_WaveGate * SampleRate));

	if (samplesPerPulse_ <= 0)
	{
		return false;
	}

	outputRate_ = samplesPerPulse_;
	inputRate_ = samplesPerPulse_ * Number;

	input.SetRate(inputRate_);
	output.SetRate(outputRate_);

	return true;
}


//------------------------------------------------------------------------------
// Run
// 非相干积分：
// 1. 对每个复数输入样本取模值 abs(x)，丢弃相位；
// 2. 对同一距离门 / 同一采样位置，跨 Number 个脉冲累加；
// 3. 输出为 real。
//------------------------------------------------------------------------------
bool RADAR_NonCoIntgr::Run()
{
	if (samplesPerPulse_ <= 0 || Number <= 0)
	{
		return false;
	}

	const int inputSize = input.GetSize();
	const int outputSize = output.GetSize();

	if (inputSize < inputRate_ || outputSize < outputRate_)
	{
		return false;
	}

	for (int sample = 0; sample < samplesPerPulse_; ++sample)
	{
		double sumAbs = 0.0;

		for (int pulse = 0; pulse < Number; ++pulse)
		{
			const int idx = pulse * samplesPerPulse_ + sample;

			// std::abs(complex) = sqrt(real^2 + imag^2)
			// 这是非相干积分的关键：先取幅度，再累加。
			sumAbs += std::abs(input[idx]);
		}

		output[sample] = sumAbs;
	}

	return true;
}