#pragma once

#include "ModelBuilder.h"

// ============================================================
// RADAR_MNDetector
// M of N Detector / 二进制积累检测器
//
// 功能：
//   一次读取 N 个 PRI 的二值检测结果，对每个 range bin 跨 N 个 PRI
//   统计检测次数；若检测次数 >= M，则输出 1，否则输出 0。
//
// 帮助文档端口速率：
//   input  rate = PRI * SampleRate * N
//   output rate = PRI * SampleRate
// ============================================================
class SYSTEMVUEMODELBUILDER_API RADAR_MNDetector : public SystemVueModelBuilder::DFModel
{
public:
	DECLARE_MODEL_INTERFACE(RADAR_MNDetector);

	RADAR_MNDetector();

	virtual bool Setup();
	virtual bool Run();

	// ============================================================
	// 端口定义
	// Port 1: input  - int，RADAR_BinaryDetector 输出的 0/1 检测序列
	// Port 2: output - int，M/N 二进制积累后的 0/1 检测结果
	// ============================================================
	SystemVueModelBuilder::CircularBuffer<int> input;
	SystemVueModelBuilder::CircularBuffer<int> output;

	// ============================================================
	// RADAR_MNDetector 帮助文档参数
	// ============================================================
	int    M;           // N 次检测中至少需要 M 次为 1
	int    N;           // 参与二进制积累的 PRI/CPI 数
	double PRI;         // Pulse Repetition Interval，单位 s
	double SampleRate;  // Sampling Rate，单位 Hz

private:
	int samplesPerPRI_; // 每个 PRI 内的样本数：round(PRI * SampleRate)
	int inputRate_;     // input 端口 rate：samplesPerPRI_ * N
	int outputRate_;    // output 端口 rate：samplesPerPRI_

private:
	bool validateAndPrepare_();
	static int calcSamplesPerPRI_(double pri, double sampleRate);
};
