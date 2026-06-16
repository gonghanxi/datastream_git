#include "RADAR_DDC.h"

#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(RADAR_DDC)
{
	SET_MODEL_DESCRIPTION("RADAR Digital Down Converter");
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
	// Model parameters，严格按照 RADAR_DDC 帮助文档设置
	// ============================================================
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
		auto p = ADD_MODEL_PARAM(ADC_NBits);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("8");
		p.SetDescription("number of bits for ADC ([2:&infin;))");
	}

	{
		auto p = ADD_MODEL_PARAM(PhaseImbalance);
		p.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		p.SetDefaultValue("0");
		p.SetDescription("phase imbalance in degrees, Q channel relative to I channel ((-&infin;:&infin;))");
	}

	{
		auto p = ADD_MODEL_PARAM(BB_DownSamplingRatio);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("20");
		p.SetDescription("the downsampling ratio from digital IF to baseband ([1:&infin;))");
	}

	{
		auto p = ADD_MODEL_PARAM(RC_ExcessBW);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("0.22");
		p.SetDescription("Excess bandwidth of raised cosine filter ((0.0:1.0))");
	}

	{
		auto p = ADD_MODEL_PARAM(Out_CenterFreq);
		p.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		p.SetDefaultValue("0");
		p.SetDescription("The center frequency of the output signal ([0:&infin;))");
	}

	// SystemVue 2020 当前工程中该宏展开函数需要显式返回值。
	return true;
}
#endif

// ============================================================
// 构造函数
// ============================================================

RADAR_DDC::RADAR_DDC()
	: IF_Freq(25000000.0)
	, IF_SamplingRate(100000000.0)
	, ADC_NBits(8)
	, PhaseImbalance(0.0)
	, BB_DownSamplingRatio(20)
	, RC_ExcessBW(0.22)
	, Out_CenterFreq(0.0)
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

ERESULT RADAR_DDC::PropagateCharacterizationFrequency()
{
	// BB_Signal 为普通 complex 数据端口，本 SystemVue 2020 CircularBuffer
	// 不暴露 characterization frequency 接口。Out_CenterFreq 在 Run 中以
	// 复指数旋转方式等效处理。
	return true;
}

bool RADAR_DDC::Setup()
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

void RADAR_DDC::applyRates_()
{
	// 帮助文档说明：BB_DownSamplingRatio 是 digital IF 到 baseband 的降采样倍率。
	// 因此 Data Flow 中每次 Run 消耗 R 个 IF envelope 样本，输出 1 个 BB complex 样本。
	IF_Signal.SetRate(static_cast<unsigned>(decim_));
	BB_Signal.SetRate(1U);

	// 注意：DComplexCircularBuffer 在当前 SystemVue 2020 ModelBuilder 版本中
	// 是 CircularBuffer<std::complex<double> >，支持 SetRate()，但不暴露
	// SetTimeStep()/SetSampleRate()/SetStartTime()。输出时间轴由调度器生成。
}

void RADAR_DDC::resetStates_()
{
	quadFirState_.clear();
	if (!quadFir_.empty()) {
		quadFirState_.resize(quadFir_.size(), Cx(0.0, 0.0));
	}
}

// ============================================================
// RADAR_QuadSample 等效 raised-cosine 抽取滤波器
// ============================================================

void RADAR_DDC::buildQuadSampleFir_()
{
	quadFir_.clear();

	const int sps = (decim_ > 0) ? decim_ : 1;

	// 与 RADAR_DUC 的 RC 插值滤波保持同一类近似。
	// 内置帮助文档说明 RC_ExcessBW 是 baseband downsampling 后的
	// raised cosine filter excess bandwidth。这里在 IF 采样率下实现
	// 等效抗混叠/低通抽取滤波，再每 R 个点取一个输出。
	//
	// 在 IF_SamplingRate=1e6、BB_DownSamplingRatio=5 时，20 us 等价于
	// 20 个 IF 输入点，也就是 4 个输出基带采样点。
	// 原 spanSymbols=30 的线性相位群时延为 30*R/2=75 个 IF 点；
	// 将 spanSymbols 调整为 22 后，群时延为 22*R/2=55 个 IF 点，
	// 正好减少约 4 个输出采样点的等效延迟，更接近内置 RADAR_QuadSample。
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

	// DDC 抽取滤波要求低通直流增益保持 1，避免破坏 Gain=2 的幅度补偿。
	if (std::fabs(sum) > 1e-30) {
		const double scale = 1.0 / sum;
		for (size_t i = 0; i < quadFir_.size(); ++i) {
			quadFir_[i] *= scale;
		}
	}
}

RADAR_DDC::Cx RADAR_DDC::runQuadSampleFir_(const Cx& x)
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

bool RADAR_DDC::Run()
{
	const int totalIn = (decim_ > 0) ? decim_ : 1;
	const unsigned lastIdx = static_cast<unsigned>(totalIn - 1);

	// 驱动输入时间轴。内置 DDC 每 R 个 IF 点输出 1 个 BB 点，输出时间
	// 通常对应当前抽取窗口末端或内部 QuadSample 抽取相位。
	(void)IF_Signal.GetTime(lastIdx, GetCount());

	const double inputFcHz = IF_Signal.GetCharacterizationFrequency();

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
		else {
			timeNow = IF_Signal.GetTime(idx, GetCount());
		}

		timeLast = timeNow;

		const SystemVueModelBuilder::EnvelopeSignal xinEnv = IF_Signal[idx];

		// --------------------------------------------------------
		// 1. FcChange + EnvToData 等效：
		// 帮助图中 IF_Signal 先进入 Fc Change，再 AtoD / EnvToData。
		// 这里先根据输入 envelope 的 characterization frequency 恢复等效实 IF。
		// --------------------------------------------------------
		double realIf = envelopeToRealIF_(xinEnv, inputFcHz, timeNow);

		// --------------------------------------------------------
		// 2. A to D：按 ADC_NBits 对等效实 IF 采样做量化。
		// --------------------------------------------------------
		realIf = applyADC_(realIf);

		// --------------------------------------------------------
		// 3. Gain=2 + RADAR_QuadSample 正交采样：
		// I =  2 * realIF * cos(wc*t)
		// Q = -2 * realIF * sin(wc*t + phi)
		// --------------------------------------------------------
		Cx z = quadSampleOneIFPoint_(realIf, timeNow);

		// --------------------------------------------------------
		// 4. RC_ExcessBW raised cosine 低通/抽取滤波。
		// --------------------------------------------------------
		yFiltered = runQuadSampleFir_(z);
	}

	// ------------------------------------------------------------
	// 5. 每 R 个输入 IF 样本输出一个 complex baseband 样本。
	// ------------------------------------------------------------
	Cx y = yFiltered;

	// ------------------------------------------------------------
	// 6. Out_CenterFreq：输出中心频率等效旋转。
	// ------------------------------------------------------------
	y = applyOutCenterFreq_(y, timeLast);

	BB_Signal[0U] = y;

	return true;
}

// ============================================================
// DDC 信号处理函数
// ============================================================

RADAR_DDC::Cx RADAR_DDC::envelopeToComplex_(
	const SystemVueModelBuilder::EnvelopeSignal& x,
	double fcHz) const
{
	if (fcHz > 0.0) {
		return x.complex();
	}

	return Cx(x.real(), 0.0);
}

double RADAR_DDC::envelopeToRealIF_(
	const SystemVueModelBuilder::EnvelopeSignal& x,
	double inputFcHz,
	double timeNow) const
{
	// 帮助图中的 Fc Change 可理解为把 envelope 信号转换到 AtoD 可采样的
	// 等效实 IF。若输入端已具有 characterization frequency，则使用输入 Fc；
	// 若未设置 Fc，则按参数 IF_Freq 作为默认 IF 频率。
	const double fc = (inputFcHz > 0.0) ? inputFcHz : IF_Freq;
	const Cx env = envelopeToComplex_(x, fc);

	if (fc <= 0.0) {
		return env.real();
	}

	const double ph = 2.0 * M_PI * fc * timeNow;
	const Cx carrier(std::cos(ph), std::sin(ph));

	// 实 IF = Re{ envelope * exp(j*2*pi*Fc*t) }
	return (env * carrier).real();
}

double RADAR_DDC::applyADC_(double x) const
{
	// 帮助图明确包含 A to D，参数为 ADC_NBits。
	// 由于帮助文档没有公开 full-scale / rounding / clipping 的具体规则，
	// 这里采用对称满量程 [-1, 1]、四舍五入到最近量化点的近似实现。
	if (ADC_NBits < 2 || ADC_NBits >= 32) {
		return x;
	}

	const double clipped = clamp(x, -1.0, 1.0);
	const int levels = 1 << ADC_NBits;
	const double step = 2.0 / static_cast<double>(levels - 1);
	const double q = std::floor((clipped + 1.0) / step + 0.5) * step - 1.0;

	return clamp(q, -1.0, 1.0);
}

RADAR_DDC::Cx RADAR_DDC::quadSampleOneIFPoint_(double realIf,
	double timeNow) const
{
	// 帮助图中 AtoD/EnvToData 后接 Gain=2，再进入 RADAR_QuadSample。
	// 正交采样近似按帮助文档的反向公式实现：
	//   VIF = VI*cos(wc*t) - VQ*sin(wc*t + phi)
	// 因此恢复时：
	//   Iraw =  2*VIF*cos(wc*t)
	//   Qraw = -2*VIF*sin(wc*t + phi)
	const double ph = 2.0 * M_PI * IF_Freq * timeNow;
	const double phi = deg2rad(PhaseImbalance);

	const double iRaw = 2.0 * realIf * std::cos(ph);
	const double qRaw = -2.0 * realIf * std::sin(ph + phi);

	return Cx(iRaw, qRaw);
}

RADAR_DDC::Cx RADAR_DDC::applyOutCenterFreq_(const Cx& x,
	double timeNow) const
{
	if (std::fabs(Out_CenterFreq) < 1e-15) {
		return x;
	}

	// 输出中心频率修正。符号后续建议用 Out_CenterFreq 非零黑盒测试确认。
	const double ph = 2.0 * M_PI * Out_CenterFreq * timeNow;
	return x * Cx(std::cos(ph), std::sin(ph));
}

// ============================================================
// 数学工具函数
// ============================================================

double RADAR_DDC::raisedCosineImpulse_(double t, double alpha)
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

double RADAR_DDC::sinc_(double x)
{
	if (std::fabs(x) < 1e-12) {
		return 1.0;
	}

	const double pix = M_PI * x;
	return std::sin(pix) / pix;
}

double RADAR_DDC::deg2rad(double x)
{
	return x * M_PI / 180.0;
}

double RADAR_DDC::clamp(double x, double lo, double hi)
{
	if (x < lo) {
		return lo;
	}
	if (x > hi) {
		return hi;
	}
	return x;
}
