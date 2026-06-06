#pragma once

#include "ModelBuilder.h"
#include <complex>
#include <cmath>

// RADAR_NonCoIntgr：Signal non-coherent Integration
// 输入：complex
// 输出：real
class SYSTEMVUEMODELBUILDER_API RADAR_NonCoIntgr : public SystemVueModelBuilder::DFModel
{
public:
	// SystemVue 模型接口宏
	DECLARE_MODEL_INTERFACE(RADAR_NonCoIntgr);

	// 构造函数
	RADAR_NonCoIntgr();

	// 系统函数
	virtual bool Setup();
	virtual bool Run();

	// ===== 端口 =====
	// input  : The input signal，complex
	// output : The output signal after non-coherent integration，real
	SystemVueModelBuilder::DComplexCircularBuffer input;
	SystemVueModelBuilder::CircularBuffer<double> output;

	// ===== 参数 =====
	double PRI_Or_WaveGate;    // Time Gate to Collect Samples，默认 10e-3 s
	int    Number;             // Number of Pulses for non-coherent integration，默认 5
	double SampleRate;         // Waveform Baseband Sampling Rate，默认 10e6 Hz

private:
	int samplesPerPulse_;      // 每个脉冲 / 每个 WaveGate 内的采样点数
	int inputRate_;            // 每次触发消耗的输入 token 数
	int outputRate_;           // 每次触发产生的输出 token 数
};
