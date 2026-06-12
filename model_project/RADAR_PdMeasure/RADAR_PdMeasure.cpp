#include "RADAR_PdMeasure.h"

#include <algorithm>
#include <cmath>


#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(RADAR_PdMeasure)
{
	SET_MODEL_DESCRIPTION("Probability of Detection Measurement");
	SET_MODEL_CATEGORY("Measurement");

	// ============================================================
	// 端口定义
	// ============================================================
	{
		auto p = ADD_MODEL_INPUT(input);
		p.SetName("input");
		p.SetDescription("The input signal");
	}

	{
		auto p = ADD_MODEL_OUTPUT(output);
		p.SetName("output");
		p.SetDescription("The Pd at each range bin");
	}

	// ============================================================
	// 参数定义
	// ============================================================
	{
		auto p = ADD_MODEL_PARAM(PRI);
		p.SetName("PRI");
		p.SetDefaultValue("1e-4");
		p.SetUnit(SystemVueModelBuilder::Units::TIME);
		p.SetDescription("Pulse Repetition Interval");
	}

	{
		auto p = ADD_MODEL_PARAM(SampleRate);
		p.SetName("SampleRate");
		p.SetDefaultValue("10e6");
		p.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		p.SetDescription("Sampling Rate");
	}

	{
		auto p = ADD_MODEL_PARAM(SimulationNumber);
		p.SetName("SimulationNumber");
		p.SetDefaultValue("1000");
		p.SetDescription("The number of simulation to measure the Pd.");
	}

	return true;
}
#endif


// ============================================================
// 构造函数
// ============================================================
RADAR_PdMeasure::RADAR_PdMeasure()
	: input()
	, output()
	, PRI(1e-4)
	, SampleRate(10e6)
	, SimulationNumber(1000)
	, rangeBinNum_(1000)
	, inputRate_(1000000)
	, outputRate_(1000)
{
}


// ============================================================
// Setup 辅助函数
// ============================================================
bool RADAR_PdMeasure::validateAndPrepare_()
{
	if (!(PRI > 0.0) || !std::isfinite(PRI))
	{
		POST_ERROR("PRI must be greater than 0.");
		return false;
	}

	if (!(SampleRate > 0.0) || !std::isfinite(SampleRate))
	{
		POST_ERROR("SampleRate must be greater than 0.");
		return false;
	}

	if (SimulationNumber < 1)
	{
		POST_ERROR("SimulationNumber must be greater than 0.");
		return false;
	}

	rangeBinNum_ = roundToInt_(PRI * SampleRate);
	if (rangeBinNum_ < 1)
	{
		POST_ERROR("PRI * SampleRate must be at least 1.");
		return false;
	}

	const long long inRate =
		static_cast<long long>(rangeBinNum_) *
		static_cast<long long>(SimulationNumber);

	if (inRate <= 0 || inRate > 2147483647LL)
	{
		POST_ERROR("Input rate PRI*SampleRate*SimulationNumber is too large.");
		return false;
	}

	inputRate_ = static_cast<int>(inRate);
	outputRate_ = rangeBinNum_;

	return true;
}


// ============================================================
// Setup：设置端口速率
// ============================================================
bool RADAR_PdMeasure::Setup()
{
	if (!validateAndPrepare_())
		return false;

	input.SetRate(static_cast<unsigned>(inputRate_));
	output.SetRate(static_cast<unsigned>(outputRate_));

	return true;
}


// ============================================================
// Run：按每个距离单元统计检测概率
// ============================================================
bool RADAR_PdMeasure::Run()
{
	// 帮助文档说明：
	//   输入速率  = PRI * SampleRate * SimulationNumber
	//   输出速率  = PRI * SampleRate
	//
	// 数据排列假设：
	//   input[sim * rangeBinNum_ + rangeBin]
	//
	// 上游 RADAR_MNDetector 输出 0/1 整数检测结果。
	// 为增强鲁棒性，这里把任意非零整数都视为“检测到”.
	const double invSim = 1.0 / static_cast<double>(SimulationNumber);

	for (int r = 0; r < rangeBinNum_; ++r)
	{
		int hitCount = 0;

		for (int s = 0; s < SimulationNumber; ++s)
		{
			const int idx = s * rangeBinNum_ + r;

			if (idx >= 0 && idx < inputRate_)
			{
				if (input[idx] != 0)
					++hitCount;
			}
		}

		const double pd = static_cast<double>(hitCount) * invSim;
		output[r] = clamp01_(pd);
	}

	return true;
}


// ============================================================
// 工具函数
// ============================================================
int RADAR_PdMeasure::roundToInt_(double x)
{
	if (x != x || !std::isfinite(x))
		return 0;

	if (x >= 0.0)
		return static_cast<int>(std::floor(x + 0.5));

	return static_cast<int>(std::ceil(x - 0.5));
}


double RADAR_PdMeasure::clamp01_(double x)
{
	if (x != x || !std::isfinite(x))
		return 0.0;

	if (x < 0.0)
		return 0.0;

	if (x > 1.0)
		return 1.0;

	return x;
}
