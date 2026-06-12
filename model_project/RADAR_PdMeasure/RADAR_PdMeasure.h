#pragma once

#include "ModelBuilder.h"
#include "CircularBuffer.h"

#include <cmath>

/*
 * RADAR_PdMeasure
 *
 * 功能说明：
 *   检测概率 Pd 测量模型（Probability of Detection Measurement）。
 *
 * 与内置帮助文档对齐：
 *   - Domain: Untimed
 *   - 输入端口：
 *       input   int，来自 M-out-of-N detector 的 0/1 检测结果
 *   - 输出端口：
 *       output  real，每个距离单元对应一个检测概率 Pd
 *
 * 参数：
 *   PRI, SampleRate, SimulationNumber
 *
 * 端口速率：
 *   RangeBinNum = PRI * SampleRate
 *   Input  rate = PRI * SampleRate * SimulationNumber
 *   Output rate = PRI * SampleRate
 *
 * 计算功能：
 *   输入是一段由 M-out-of-N detector 生成的 0/1 检测结果序列。
 *   对每个距离单元 range bin，在 SimulationNumber 次 Monte-Carlo 仿真中
 *   统计检测为 1 的次数，并除以仿真次数，得到该距离单元的检测概率：
 *
 *     output[range] =
 *         count_nonzero(input[sim * RangeBinNum + range]) / SimulationNumber
 */

class SYSTEMVUEMODELBUILDER_API RADAR_PdMeasure : public SystemVueModelBuilder::DFModel
{
public:
	DECLARE_MODEL_INTERFACE(RADAR_PdMeasure);

	RADAR_PdMeasure();

	virtual bool Setup();
	virtual bool Run();

	// ============================================================
	// 端口定义
	// Port 1: input，输入整数检测结果，通常为 0/1
	// Port 2: output，输出每个距离单元的检测概率 Pd
	// ============================================================
	SystemVueModelBuilder::CircularBuffer<int>    input;
	SystemVueModelBuilder::DoubleCircularBuffer   output;

	// ============================================================
	// 参数定义
	// ============================================================
	double PRI;               // 脉冲重复间隔，单位 s
	double SampleRate;        // 采样率，单位 Hz
	int    SimulationNumber;  // 用于统计 Pd 的仿真次数 / Monte-Carlo 次数

private:
	int rangeBinNum_;         // 每次仿真的距离单元数，等于 PRI * SampleRate
	int inputRate_;           // 输入速率，等于 rangeBinNum_ * SimulationNumber
	int outputRate_;          // 输出速率，等于 rangeBinNum_

private:
	bool validateAndPrepare_();

	static int roundToInt_(double x);
	static double clamp01_(double x);
};
