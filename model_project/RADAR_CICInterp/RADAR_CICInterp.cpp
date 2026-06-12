#include "RADAR_CICInterp.h"

#include <algorithm>
#include <sstream>
#include <cmath>

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(RADAR_CICInterp)
{
	SET_MODEL_DESCRIPTION("RADAR CIC Interpolation");
	SET_MODEL_CATEGORY("Transmitters");

	// ============================================================
	// 端口
	// ============================================================
	{
		auto p = ADD_MODEL_INPUT(input);
		p.SetName("input");
		p.SetDescription("complex signal to be interpolated");
	}

	{
		auto p = ADD_MODEL_OUTPUT(output);
		p.SetName("output");
		p.SetDescription("interpolated complex signal");
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
		p.SetDescription("the Interpolation ratio applied to the input signal");
	}

	{
		auto p = ADD_MODEL_PARAM(DiffDelay);
		p.SetName("DiffDelay");
		p.SetDefaultValue("1");
		p.SetDescription("Differential delay. Changes both the shape and number of nulls in the filter response. Also affects the null locations.");
	}

	{
		auto p = ADD_MODEL_PARAM(Phase);
		p.SetName("Phase");
		p.SetDefaultValue("0");
		p.SetDescription("where to put the input in the output block");
	}

	{
		auto p = ADD_MODEL_PARAM(Fill);
		p.SetName("Fill");
		p.SetDefaultValue("0");
		p.SetDescription("value to fill the output block");
	}

	return true;
}
#endif

// ============================================================
// 构造函数
// ============================================================
RADAR_CICInterp::RADAR_CICInterp()
	: input()
	, output()
	, Order(5)
	, Ratio(2)
	, DiffDelay(1)
	, Phase(0)
	, Fill(0.0, 0.0)
	, order_(5)
	, ratio_(2)
	, diffDelay_(1)
	, phase_(0)
	, gainScale_(1.0)
{
}

// ============================================================
// 参数检查
// ============================================================
bool RADAR_CICInterp::validateParameters_()
{
	if (Order <= 0)
	{
		POST_ERROR("Order must be a positive integer.");
		return false;
	}

	if (Ratio <= 0)
	{
		POST_ERROR("Ratio must be a positive integer.");
		return false;
	}

	if (DiffDelay <= 0)
	{
		POST_ERROR("DiffDelay must be a positive integer.");
		return false;
	}

	if (Phase < 0 || Phase >= Ratio)
	{
		POST_ERROR("Phase must be in the range [0, Ratio-1].");
		return false;
	}

	order_ = Order;
	ratio_ = Ratio;
	diffDelay_ = DiffDelay;
	phase_ = Phase;

	updateGainScale_();

	return true;
}

// ============================================================
// 状态清零
// ============================================================
void RADAR_CICInterp::resetStates_()
{
	combDelay_.assign(static_cast<size_t>(order_),
		std::vector<Cx>(static_cast<size_t>(diffDelay_), Cx(0.0, 0.0)));

	combWriteIndex_.assign(static_cast<size_t>(order_), 0);
	integratorState_.assign(static_cast<size_t>(order_), Cx(0.0, 0.0));
}

// ============================================================
// Setup：设置 Data Flow 多速率调度
// 每次 firing 消耗 1 个输入 token，产生 Ratio 个输出 token。
// ============================================================
bool RADAR_CICInterp::Setup()
{
	if (!validateParameters_())
		return false;

	input.SetRate(1u);
	output.SetRate(static_cast<unsigned>(ratio_));

	// Setup 中先分配一次，Initialize 中会再次清零，保证仿真重启状态一致。
	resetStates_();

	return true;
}

bool RADAR_CICInterp::Initialize()
{
	if (!validateParameters_())
		return false;

	resetStates_();
	return true;
}

bool RADAR_CICInterp::Finalize()
{
	return true;
}

// ============================================================
// N 级低速 comb
// 每一级为：y[n] = x[n] - x[n-M]
// DiffDelay = M，comb 运行在低速 fs/R。
// ============================================================

// ============================================================
// CIC 内插增益归一化
// ------------------------------------------------------------
// 对 N 级 CIC interpolator：
//   原始直流增益 = (Ratio * DiffDelay)^Order / Ratio
// 因此归一化系数为：
//   gainScale = Ratio / (Ratio * DiffDelay)^Order
// 默认值下 gainScale = 2 / 2^5 = 1/16。
// ============================================================
void RADAR_CICInterp::updateGainScale_()
{
	const double base = static_cast<double>(ratio_) * static_cast<double>(diffDelay_);

	if (order_ <= 0 || ratio_ <= 0 || diffDelay_ <= 0 || base <= 0.0)
	{
		gainScale_ = 1.0;
		return;
	}

	const double rawGain = std::pow(base, static_cast<double>(order_)) / static_cast<double>(ratio_);
	if (rawGain > 0.0 && std::isfinite(rawGain))
		gainScale_ = 1.0 / rawGain;
	else
		gainScale_ = 1.0;
}

// ============================================================
// N 级低速 comb
// 每一级为：y[n] = x[n] - x[n-M]
// DiffDelay = M，comb 运行在低速 fs/R。
// ============================================================
RADAR_CICInterp::Cx RADAR_CICInterp::runCombStages_(const Cx& x)
{
	Cx v = x;

	for (int s = 0; s < order_; ++s)
	{
		const int idx = combWriteIndex_[static_cast<size_t>(s)];
		const Cx delayed = combDelay_[static_cast<size_t>(s)][static_cast<size_t>(idx)];

		// 先把当前级输入写入延迟线，再输出差分结果。
		combDelay_[static_cast<size_t>(s)][static_cast<size_t>(idx)] = v;
		combWriteIndex_[static_cast<size_t>(s)] = (idx + 1) % diffDelay_;

		v = v - delayed;
	}

	return v;
}

// ============================================================
// N 级高速 integrator
// 每一级为：y[m] = y[m-1] + x[m]
// integrator 运行在高速 fs，因此每个输入 token 会执行 Ratio 次。
// ============================================================
RADAR_CICInterp::Cx RADAR_CICInterp::runIntegratorStages_(const Cx& x)
{
	Cx v = x;

	for (int s = 0; s < order_; ++s)
	{
		integratorState_[static_cast<size_t>(s)] += v;
		v = integratorState_[static_cast<size_t>(s)];
	}

	return v;
}

// ============================================================
// Run：CIC 内插主流程
// 1. 读取 1 个输入复数 token；
// 2. 低速侧经过 Order 级 comb；
// 3. specified-stuffer 产生 Ratio 个高速 token：
//    第 Phase 个位置放 comb 输出，其余位置放 Fill；
// 4. 每个高速 token 经过 Order 级 integrator；
// 5. 输出 Ratio 个复数 token。
// ============================================================
bool RADAR_CICInterp::Run()
{
	// 如果参数在运行前被外部修改过，这里做一次保护性检查。
	// 帮助文档标注 Runtime Tunable = NO，正常情况下不会变化。
	if (Order != order_ || Ratio != ratio_ || DiffDelay != diffDelay_ || Phase != phase_)
	{
		if (!validateParameters_())
			return false;

		output.SetRate(static_cast<unsigned>(ratio_));
		resetStates_();
	}

	const Cx x = input[0];

	// 低速 comb 部分只运行一次。
	const Cx combOut = runCombStages_(x);

	// specified-stuffer + 高速 integrator。
	for (int r = 0; r < ratio_; ++r)
	{
		const Cx stuffed = (r == phase_) ? combOut : Fill;
		output[r] = runIntegratorStages_(stuffed) * gainScale_;
	}

	return true;
}
