#include "RADAR_MNDetector.h"

#include <cmath>

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(RADAR_MNDetector)
{
	SET_MODEL_DESCRIPTION("M of N Detector");
	SET_MODEL_CATEGORY("Signal Processing");

	// ============================================================
	// 端口
	// ============================================================
	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(input);
		port.SetName("input");
		port.SetDescription("The input signal");
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
		port.SetName("output");
		port.SetDescription("The detected result after m-outof-n detector");
	}

	// ============================================================
	// 参数
	// ============================================================
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(M);
		param.SetName("M");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("2");
		param.SetDescription("M is the number of target decisions of N times detection");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(N);
		param.SetName("N");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("5");
		param.SetDescription("N is the number of detection");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(PRI);
		param.SetName("PRI");
		param.SetUnit(SystemVueModelBuilder::Units::TIME);
		param.SetDefaultValue("1e-4");
		param.SetDescription("Pulse Repetition Interval");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(SampleRate);
		param.SetName("SampleRate");
		param.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		param.SetDefaultValue("10e6");
		param.SetDescription("Sampling Rate");
	}

	return true;
}
#endif

// ============================================================
// 构造函数
// ============================================================
RADAR_MNDetector::RADAR_MNDetector()
	: input()
	, output()
	, M(2)
	, N(5)
	, PRI(1.0e-4)
	, SampleRate(10.0e6)
	, samplesPerPRI_(1000)
	, inputRate_(5000)
	, outputRate_(1000)
{
}

// ============================================================
// 根据 PRI 和 SampleRate 计算一个 PRI 内的采样点数
//
// 帮助文档给出的端口速率为 PRI * SampleRate。
// SystemVue 的端口 rate 必须是整数，因此这里采用四舍五入。
// 推荐验证时让 PRI * SampleRate 正好为整数，避免不同取整方式引入差异。
// ============================================================
int RADAR_MNDetector::calcSamplesPerPRI_(double pri, double sampleRate)
{
	const double v = pri * sampleRate;
	if (v <= 0.0) {
		return 0;
	}

	return static_cast<int>(std::floor(v + 0.5));
}

// ============================================================
// 参数检查与 rate 预计算
// ============================================================
bool RADAR_MNDetector::validateAndPrepare_()
{
	if (M < 1)
	{
		POST_ERROR("M must be >= 1.");
		return false;
	}

	if (N < 1)
	{
		POST_ERROR("N must be >= 1.");
		return false;
	}

	if (M > N)
	{
		POST_ERROR("M must be <= N.");
		return false;
	}

	if (PRI <= 0.0)
	{
		POST_ERROR("PRI must be > 0.");
		return false;
	}

	if (SampleRate <= 0.0)
	{
		POST_ERROR("SampleRate must be > 0.");
		return false;
	}

	samplesPerPRI_ = calcSamplesPerPRI_(PRI, SampleRate);
	if (samplesPerPRI_ < 1)
	{
		POST_ERROR("PRI * SampleRate must be >= 1 sample.");
		return false;
	}

	// 如果 PRI*SampleRate 不是整数，给出提示，但仍按四舍五入后的整数 rate 执行。
	// 常规 SystemVue 雷达链路中该乘积一般应设置为整数。
	{
		const double exact = PRI * SampleRate;
		const double diff = std::fabs(exact - static_cast<double>(samplesPerPRI_));
		if (diff > 1.0e-9)
		{
			POST_WARNING("PRI * SampleRate is not an integer. Port rate is rounded to nearest integer.");
		}
	}

	outputRate_ = samplesPerPRI_;
	inputRate_ = samplesPerPRI_ * N;

	if (inputRate_ < outputRate_ || inputRate_ <= 0 || outputRate_ <= 0)
	{
		POST_ERROR("Invalid port rate calculated from PRI, SampleRate and N.");
		return false;
	}

	return true;
}

// ============================================================
// Setup
// 设置端口 rate：
//   input  = PRI * SampleRate * N
//   output = PRI * SampleRate
// ============================================================
bool RADAR_MNDetector::Setup()
{
	if (!validateAndPrepare_()) {
		return false;
	}

	input.SetRate(static_cast<unsigned>(inputRate_));
	output.SetRate(static_cast<unsigned>(outputRate_));

	return true;
}

// ============================================================
// Run
//
// 输入数据排列方式：
//   第 0 个 PRI: input[0 ... L-1]
//   第 1 个 PRI: input[L ... 2L-1]
//   ...
//   第 N-1 个 PRI: input[(N-1)L ... N*L-1]
//
// 其中 L = samplesPerPRI_。
// 对每个 range bin i，统计 N 个 PRI 中 input[p*L+i] 是否为 1。
// 若命中次数 >= M，output[i] = 1，否则 output[i] = 0。
// ============================================================
bool RADAR_MNDetector::Run()
{
	const int L = samplesPerPRI_;

	for (int i = 0; i < L; ++i)
	{
		int hitCount = 0;

		for (int p = 0; p < N; ++p)
		{
			const int idx = p * L + i;

			// 帮助文档说明输入来自 RADAR_BinaryDetector，数据应为 0/1。
			// 这里严格按“为 1 表示检测到目标”的规则统计。
			if (input[idx] == 1)
			{
				++hitCount;
			}
		}

		output[i] = (hitCount >= M) ? 1 : 0;
	}

	return true;
}
