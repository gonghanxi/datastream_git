#include "RADAR_QuadSample.h"

#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(RADAR_QuadSample)
{
	SET_MODEL_DESCRIPTION("RADAR quadrature sampling");
	SET_MODEL_CATEGORY("Receiver");

	// ============================================================
	// Ports，严格对应帮助文档
	// ============================================================
	{
		auto p = ADD_MODEL_INPUT(IF_Signal);
		p.SetDescription("IF signal");
	}

	{
		auto p = ADD_MODEL_OUTPUT(BB_Signal);
		p.SetDescription("BB signal");
	}

	// ============================================================
	// Model parameters，严格按照 RADAR_QuadSample 帮助文档设置
	// ============================================================
	{
		auto p = ADD_MODEL_PARAM(BB_DownSamplingRatio);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("20");
		p.SetDescription("the downsampling ratio from digital IF to baseband ([1:&infin;))");
	}

	{
		auto p = ADD_MODEL_PARAM(IF_Freq);
		p.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		p.SetDefaultValue("25000000");
		p.SetDescription("IF carrier frequency ((0:&infin;))");
	}

	{
		auto p = ADD_MODEL_PARAM(IF_SamplingRate);
		p.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		p.SetDefaultValue("100000000");
		p.SetDescription("Digital IF signal sampling rate ((0:&infin;))");
	}

	{
		auto p = ADD_MODEL_PARAM(Out_CenterFreq);
		p.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		p.SetDefaultValue("0");
		p.SetDescription("The center frequency of the output signal ([0:&infin;))");
	}

	{
		auto p = ADD_MODEL_PARAM(PhaseImbalance);
		p.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		p.SetDefaultValue("0");
		p.SetDescription("phase imbalance in degrees, Q channel relative to I channel ((-&infin;:&infin;))");
	}

	{
		auto p = ADD_MODEL_PARAM(RC_ExcessBW);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("0.22");
		p.SetDescription("Excess bandwidth of raised cosine filter ((0.0:1.0))");
	}

	// SystemVue 2020 当前工程中该宏展开函数需要显式返回值。
	return true;
}
#endif

// ============================================================
// 构造函数
// ============================================================

RADAR_QuadSample::RADAR_QuadSample()
	: BB_DownSamplingRatio(20)
	, IF_Freq(25000000.0)
	, IF_SamplingRate(100000000.0)
	, Out_CenterFreq(0.0)
	, PhaseImbalance(0.0)
	, RC_ExcessBW(0.22)
	, inputSampleRateHz_(100000000.0)
	, inputTimeStepSec_(1.0 / 100000000.0)
	, outputSampleRateHz_(5000000.0)
	, outputTimeStepSec_(1.0 / 5000000.0)
	, decim_(20)
{
}

// ============================================================
// Setup 与载频传播
// ============================================================

ERESULT RADAR_QuadSample::PropagateCharacterizationFrequency()
{
	// BB_Signal 为普通 complex 数据端口，当前 SystemVue 2020
	// DComplexCircularBuffer 不暴露 characterization frequency 接口。
	// Out_CenterFreq 在 Run 中以复指数旋转方式等效处理。
	return true;
}

bool RADAR_QuadSample::Setup()
{
	decim_ = (BB_DownSamplingRatio > 0) ? BB_DownSamplingRatio : 1;
	if (decim_ < 1) {
		decim_ = 1;
	}

	inputSampleRateHz_ = (IF_SamplingRate > 0.0) ? IF_SamplingRate : 0.0;
	inputTimeStepSec_ = (inputSampleRateHz_ > 0.0) ? (1.0 / inputSampleRateHz_) : 0.0;

	outputSampleRateHz_ = (inputSampleRateHz_ > 0.0) ?
		(inputSampleRateHz_ / static_cast<double>(decim_)) : 0.0;
	outputTimeStepSec_ = (outputSampleRateHz_ > 0.0) ?
		(1.0 / outputSampleRateHz_) : 0.0;

	applyRates_();
	buildQuadSampleFir_();
	resetStates_();

	return true;
}

void RADAR_QuadSample::applyRates_()
{
	// 帮助文档说明：BB_DownSamplingRatio 是 digital IF 到 baseband 的降采样倍率。
	// 因此每次 Run 消耗 R 个 IF real 样本，输出 1 个 BB complex 样本。
	IF_Signal.SetRate(static_cast<unsigned>(decim_));
	BB_Signal.SetRate(1U);

	// 注意：DComplexCircularBuffer 在当前 SystemVue 2020 ModelBuilder 版本中
	// 是 CircularBuffer<std::complex<double> >，支持 SetRate()，但不暴露
	// SetTimeStep()/SetSampleRate()/SetStartTime()。输出时间轴由调度器生成。
}

void RADAR_QuadSample::resetStates_()
{
	quadFirState_.clear();
	if (!quadFir_.empty()) {
		quadFirState_.resize(quadFir_.size(), Cx(0.0, 0.0));
	}
}

// ============================================================
// FIR_Cx：raised-cosine 复数低通滤波器
// ============================================================

void RADAR_QuadSample::buildQuadSampleFir_()
{
	quadFir_.clear();

	const int sps = (decim_ > 0) ? decim_ : 1;

	// 根据 RADAR_DDC V2 的验证经验，spanSymbols=22 时的实部/虚部形状
	// 与内置 QuadSample 子网络最接近。这里保持对称 FIR，不做分数移动，
	// 避免破坏线性相位和 I/Q 波形形状。
	const int spanSymbols = 22;
	const int nTaps = spanSymbols * sps + 1;
	const int mid = nTaps / 2;

	quadFir_.resize(nTaps, 0.0);

	const double alpha = clamp(RC_ExcessBW, 0.0, 1.0);
	double sum = 0.0;

	for (int n = 0; n < nTaps; ++n) {
		const double t = static_cast<double>(n - mid) / static_cast<double>(sps);
		quadFir_[n] = raisedCosineImpulse_(t, alpha);
		sum += quadFir_[n];
	}

	// FIR_Cx 低通直流增益保持 1。独立 QuadSample 内部不额外乘 2；
	// 在 RADAR_DDC 子网络中，Gain=2 位于 QuadSample 外部。
	if (std::fabs(sum) > 1e-30) {
		const double scale = 1.0 / sum;
		for (size_t i = 0; i < quadFir_.size(); ++i) {
			quadFir_[i] *= scale;
		}
	}
}

RADAR_QuadSample::Cx RADAR_QuadSample::runQuadSampleFir_(const Cx& x)
{
	if (quadFir_.empty()) {
		return x;
	}

	if (quadFirState_.size() != quadFir_.size()) {
		quadFirState_.clear();
		quadFirState_.resize(quadFir_.size(), Cx(0.0, 0.0));
	}

	quadFirState_.push_front(x);
	while (quadFirState_.size() > quadFir_.size()) {
		quadFirState_.pop_back();
	}

	Cx y(0.0, 0.0);
	for (size_t i = 0; i < quadFir_.size(); ++i) {
		y += quadFir_[i] * quadFirState_[i];
	}

	return y;
}

// ============================================================
// Run
// ============================================================

bool RADAR_QuadSample::Run()
{
	const int totalIn = (decim_ > 0) ? decim_ : 1;

	// 普通 real circular buffer 不一定暴露样本时间接口，因此这里使用
	// IF_SamplingRate 和 GetCount() 构造确定性时间轴。该时间轴与帮助文档
	// 中 IF_SamplingRate 是输入采样率的定义一致。
	Cx yFiltered(0.0, 0.0);
	double timeLast = 0.0;

	for (int i = 0; i < totalIn; ++i) {
		const unsigned idx = static_cast<unsigned>(i);

		double timeNow = 0.0;
		if (inputTimeStepSec_ > 0.0) {
			timeNow =
				(static_cast<double>(GetCount()) * static_cast<double>(totalIn) +
					static_cast<double>(i)) * inputTimeStepSec_;
		}

		timeLast = timeNow;

		const double realIf = IF_Signal[idx];

		// --------------------------------------------------------
		// 1. 数字正交采样：
		// 帮助图显示 IF_Signal 分两路分别与正交本振相乘，Q 路前有 Gain=-1。
		// 独立 QuadSample 内部不乘 2；若测试理论 IF：
		//   x(t)=I*cos(wt)-Q*sin(wt)
		// 则需要在外部接 Gain=2 才能恢复原始 I/Q 幅度。
		// --------------------------------------------------------
		const Cx z = quadSampleOneIFPoint_(realIf, timeNow);

		// --------------------------------------------------------
		// 2. FIR_Cx：对 full-rate complex I/Q 进行 raised-cosine 低通滤波。
		// --------------------------------------------------------
		yFiltered = runQuadSampleFir_(z);
	}

	// ------------------------------------------------------------
	// 3. DownSample：每 R 个输入 IF 样本输出 1 个 complex baseband 样本。
	// ------------------------------------------------------------
	Cx y = yFiltered;

	// ------------------------------------------------------------
	// 4. Out_CenterFreq：输出中心频率等效旋转。
	// ------------------------------------------------------------
	y = applyOutCenterFreq_(y, timeLast);

	BB_Signal[0U] = y;

	return true;
}

// ============================================================
// 正交采样与输出中心频率处理
// ============================================================

RADAR_QuadSample::Cx RADAR_QuadSample::quadSampleOneIFPoint_(double realIf,
	double timeNow) const
{
	// 帮助文档公式：
	//   VIF(t) = VI(t)*cos(wc*t) - VQ(t)*sin(wc*t + phi)
	// 内置 QuadSample 做反向正交解调。帮助图中 Q 路通过 Gain=-1，
	// 因此等效为：
	//   Re_raw =  realIF * cos(wc*t)
	//   Im_raw = -realIF * sin(wc*t + phi)
	// 注意：Gain=2 不在独立 QuadSample 内，而在 DDC 外层。
	const double ph = 2.0 * M_PI * IF_Freq * timeNow;
	const double phi = deg2rad(PhaseImbalance);

	const double iRaw = realIf * std::cos(ph);
	const double qRaw = -realIf * std::sin(ph + phi);

	return Cx(iRaw, qRaw);
}

RADAR_QuadSample::Cx RADAR_QuadSample::applyOutCenterFreq_(const Cx& x,
	double timeNow) const
{
	if (std::fabs(Out_CenterFreq) < 1e-15) {
		return x;
	}

	// 输出中心频率修正。若后续 Out_CenterFreq 黑盒验证发现旋转方向相反，
	// 只需要把这里 ph 前的符号取反。
	const double ph = 2.0 * M_PI * Out_CenterFreq * timeNow;
	return x * Cx(std::cos(ph), std::sin(ph));
}

// ============================================================
// 数学工具函数
// ============================================================

double RADAR_QuadSample::raisedCosineImpulse_(double t, double alpha)
{
	if (std::fabs(t) < 1e-12) {
		return 1.0;
	}

	if (alpha > 1e-12) {
		const double singular = 1.0 / (2.0 * alpha);
		if (std::fabs(std::fabs(t) - singular) < 1e-10) {
			return 0.5 * sinc_(singular);
		}
	}

	const double pix = M_PI * t;
	const double num = std::sin(pix) / pix;
	const double den = 1.0 - 4.0 * alpha * alpha * t * t;

	if (std::fabs(den) < 1e-12) {
		return 0.0;
	}

	return num * std::cos(M_PI * alpha * t) / den;
}

double RADAR_QuadSample::sinc_(double x)
{
	if (std::fabs(x) < 1e-12) {
		return 1.0;
	}

	const double pix = M_PI * x;
	return std::sin(pix) / pix;
}

double RADAR_QuadSample::deg2rad(double x)
{
	return x * M_PI / 180.0;
}

double RADAR_QuadSample::clamp(double x, double lo, double hi)
{
	if (x < lo) {
		return lo;
	}
	if (x > hi) {
		return hi;
	}
	return x;
}
