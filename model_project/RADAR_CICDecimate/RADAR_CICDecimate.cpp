#include "RADAR_CICDecimate.h"

#include <cmath>
#include <algorithm>

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(RADAR_CICDecimate)
{
	SET_MODEL_DESCRIPTION("RADAR CIC Decimation");
	SET_MODEL_CATEGORY("Receiver");

	// ============================================================
	// 端口
	// ============================================================
	{
		auto p = ADD_MODEL_INPUT(input);
		p.SetName("input");
		p.SetDescription("complex signal to be decimated");
	}

	{
		auto p = ADD_MODEL_OUTPUT(output);
		p.SetName("output");
		p.SetDescription("decimated complex signal");
	}

	// ============================================================
	// 参数
	// ============================================================
	{
		auto p = ADD_MODEL_PARAM(Order);
		p.SetName("Order");
		p.SetDefaultValue("5");
		p.SetDescription("the concatenation order of the CIC filter");
	}

	{
		auto p = ADD_MODEL_PARAM(Ratio);
		p.SetName("Ratio");
		p.SetDefaultValue("2");
		p.SetDescription("the Decimation ratio applied to the input signal");
	}

	{
		auto p = ADD_MODEL_PARAM(DiffDelay);
		p.SetName("DiffDelay");
		p.SetDefaultValue("1");
		p.SetDescription("Differential delay. Changes both the shape and number of nulls in the filter response. Also affects the null locations. In practice, it is usually held to one or two");
	}

	{
		auto p = ADD_MODEL_PARAM(Phase);
		p.SetName("Phase");
		p.SetDefaultValue("0");
		p.SetDescription("downsampling phase");
	}

	return true;
}
#endif

// ============================================================
// 构造函数
// ============================================================
RADAR_CICDecimate::RADAR_CICDecimate()
	: input()
	, output()
	, Order(5)
	, Ratio(2)
	, DiffDelay(1)
	, Phase(0)
	, order_(5)
	, ratio_(2)
	, diffDelay_(1)
	, phase_(0)
	, gainScale_(1.0 / 32.0)
{
}

// ============================================================
// 参数检查与内部缓存准备
// ============================================================
bool RADAR_CICDecimate::validateAndPrepare_()
{
	order_ = Order;
	ratio_ = Ratio;
	diffDelay_ = DiffDelay;
	phase_ = Phase;

	// 帮助文档中 Order / Ratio / DiffDelay 均为正整数。
	// 为了避免 Data Flow 调度异常，这里对非法值做保护性限制。
	if (order_ < 1) {
		order_ = 1;
	}

	if (ratio_ < 1) {
		ratio_ = 1;
	}

	if (diffDelay_ < 1) {
		diffDelay_ = 1;
	}

	phase_ = clampInt_(phase_, 0, ratio_ - 1);

	gainScale_ = computeGainScale_();

	return true;
}

void RADAR_CICDecimate::resetStates_()
{
	integratorState_.assign(static_cast<size_t>(order_), Cx(0.0, 0.0));

	combDelay_.clear();
	combDelay_.resize(static_cast<size_t>(order_));

	for (int s = 0; s < order_; ++s)
	{
		combDelay_[static_cast<size_t>(s)].clear();
		for (int d = 0; d < diffDelay_; ++d)
		{
			combDelay_[static_cast<size_t>(s)].push_back(Cx(0.0, 0.0));
		}
	}
}

// ============================================================
// SystemVue 生命周期函数
// ============================================================
bool RADAR_CICDecimate::Setup()
{
	if (!validateAndPrepare_()) {
		return false;
	}

	// 帮助文档：每次 firing 消耗 Ratio 个输入 token，产生 1 个输出 token。
	input.SetRate(static_cast<unsigned>(ratio_));
	output.SetRate(1u);

	resetStates_();

	return true;
}

bool RADAR_CICDecimate::Initialize()
{
	resetStates_();
	return true;
}

bool RADAR_CICDecimate::Finalize()
{
	return true;
}

bool RADAR_CICDecimate::UpdateDynamicParameters()
{
	// 参数在运行中变化时，重新设置速率与内部状态。
	return Setup();
}

bool RADAR_CICDecimate::Run()
{
	// ============================================================
	// 内置模块实现方式推测：
	//   1. Ratio 个高速输入样本全部进入 Order 级积分器；
	//   2. Phase 决定本组 Ratio 个积分结果中哪一个被抽取；
	//   3. 抽取后的低速样本进入 Order 级 comb；
	//   4. 按 CIC 直流增益 (Ratio*DiffDelay)^Order 做归一化。
	//
	// 帮助文档给出的抽取相位公式为：
	//   y[n] = x[Ratio*n + Phase]
	// 这里的 x 对应抽取器前的高速 CIC 积分链输出。
	// ============================================================

	Cx selected(0.0, 0.0);

	// ============================================================
	// 与内置模块对齐的关键点：
	//   帮助文档把 Phase 描述为 y[n] = x[Ratio*n + Phase]。
	//   但在 SystemVue Data Flow 的一次 firing 中，当前窗口 input[0..R-1]
	//   对应的是已经凑齐后的 Ratio 个输入 token。黑盒默认参数常量输入结果表明：
	//       Phase = 0 时，内置选择当前 firing 内最后一个高速积分结果；
	//       Phase = Ratio-1 时，才选择当前 firing 内第一个高速积分结果。
	//   因此内部数组下标需要使用：
	//       takeIndex = Ratio - 1 - Phase
	// ============================================================
	const int takeIndex = ratio_ - 1 - phase_;

	for (int r = 0; r < ratio_; ++r)
	{
		const Cx x = input[r];
		const Cx intOut = runIntegratorStages_(x);

		if (r == takeIndex)
		{
			selected = intOut;
		}
	}

	const Cx combOut = runCombStages_(selected);
	output[0] = combOut * gainScale_;

	return true;
}

// ============================================================
// CIC 核心处理函数
// ============================================================
RADAR_CICDecimate::Cx RADAR_CICDecimate::runIntegratorStages_(const Cx& x)
{
	Cx v = x;

	for (int s = 0; s < order_; ++s)
	{
		const size_t idx = static_cast<size_t>(s);
		integratorState_[idx] += v;
		v = integratorState_[idx];
	}

	return v;
}

RADAR_CICDecimate::Cx RADAR_CICDecimate::runCombStages_(const Cx& x)
{
	Cx v = x;

	for (int s = 0; s < order_; ++s)
	{
		const size_t idx = static_cast<size_t>(s);

		Cx delayed(0.0, 0.0);
		if (!combDelay_[idx].empty())
		{
			delayed = combDelay_[idx].front();
			combDelay_[idx].pop_front();
		}

		combDelay_[idx].push_back(v);

		const Cx y = v - delayed;
		v = y;
	}

	return v;
}

// ============================================================
// 增益归一化
// ============================================================
double RADAR_CICDecimate::computeGainScale_() const
{
	// 标准 CIC Decimator 的直流增益为：
	//   Gain = (Ratio * DiffDelay)^Order
	// 结合 RADAR_CICInterp 的实测，内置 CIC 模块采用归一化输出，
	// 因此这里使用其倒数。
	const double base = static_cast<double>(std::max(1, ratio_))
		* static_cast<double>(std::max(1, diffDelay_));

	double gain = 1.0;
	for (int i = 0; i < std::max(1, order_); ++i)
	{
		gain *= base;
	}

	if (gain <= 0.0) {
		return 1.0;
	}

	return 1.0 / gain;
}

int RADAR_CICDecimate::clampInt_(int x, int lo, int hi)
{
	if (hi < lo) {
		return lo;
	}

	if (x < lo) {
		return lo;
	}

	if (x > hi) {
		return hi;
	}

	return x;
}
