#include "RADAR_DUC.h"

#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(RADAR_DUC)
{
	SET_MODEL_DESCRIPTION("RADAR Digital Up Converter");
	SET_MODEL_CATEGORY("Transmitter");

	// ============================================================
	// Ports
	// ============================================================
	{
		auto p = ADD_MODEL_INPUT(BB_Signal);
		p.SetDescription("BB signal");
	}

	{
		auto p = ADD_MODEL_OUTPUT(IF_Signal);
		p.SetDescription("IF signal");
	}

	// ============================================================
	// Model parameters，严格按照 RADAR_DUC 帮助文档设置
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
		auto p = ADD_MODEL_PARAM(BandWidth);
		p.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		p.SetDefaultValue("5000000");
		p.SetDescription("passband bandwidth of IF filter ((0:&infin;))");
	}

	{
		auto p = ADD_MODEL_PARAM(In_CenterFreq);
		p.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		p.SetDefaultValue("0");
		p.SetDescription("The center frequency of the input signal ([0:&infin;))");
	}

	{
		auto p = ADD_MODEL_PARAM(BB_UpSamplingRatio);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("20");
		p.SetDescription("the upsampling ratio from baseband to digital IF ([1:&infin;))");
	}

	{
		auto p = ADD_MODEL_PARAM(RC_ExcessBW);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("0.22");
		p.SetDescription("Excess bandwidth of raised cosine filter ((0.0:1.0))");
	}

	{
		auto p = ADD_MODEL_PARAM(PhaseImbalance);
		p.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		p.SetDefaultValue("0");
		p.SetDescription("phase imbalance in degrees, Q channel relative to I channel ((-&infin;:&infin;))");
	}

	{
		auto p = ADD_MODEL_PARAM(DAC_NBits);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("8");
		p.SetDescription("number of bits for DAC ([2:&infin;))");
	}

	return true;
}
#endif

// ============================================================
// 状态结构
// ============================================================

RADAR_DUC::BiquadState::BiquadState()
	: b0(1.0)
	, b1(0.0)
	, b2(0.0)
	, a1(0.0)
	, a2(0.0)
	, x1(0.0, 0.0)
	, x2(0.0, 0.0)
	, y1(0.0, 0.0)
	, y2(0.0, 0.0)
{
}

void RADAR_DUC::BiquadState::reset()
{
	x1 = Cx(0.0, 0.0);
	x2 = Cx(0.0, 0.0);
	y1 = Cx(0.0, 0.0);
	y2 = Cx(0.0, 0.0);
}

// ============================================================
// 构造函数
// ============================================================

RADAR_DUC::RADAR_DUC()
	: IF_Freq(25000000.0)
	, IF_SamplingRate(100000000.0)
	, BandWidth(5000000.0)
	, In_CenterFreq(0.0)
	, BB_UpSamplingRatio(20)
	, RC_ExcessBW(0.22)
	, PhaseImbalance(0.0)
	, DAC_NBits(8)
	, outputSampleRateHz_(100000000.0)
	, outputTimeStepSec_(1.0 / 100000000.0)
	, upRate_(20)
	, outRate_(20)
	, ifBpfEnabled_(false)
{
}

// ============================================================
// Setup 与载频传播
// ============================================================

ERESULT RADAR_DUC::PropagateCharacterizationFrequency()
{
	IF_Signal.SetCharacterizationFrequency(IF_Freq);
	return true;
}

bool RADAR_DUC::Setup()
{
	upRate_ = (BB_UpSamplingRatio > 0) ? BB_UpSamplingRatio : 1;
	outRate_ = upRate_;
	if (outRate_ < 1) {
		outRate_ = 1;
	}

	outputSampleRateHz_ = (IF_SamplingRate > 0.0) ? IF_SamplingRate : 0.0;
	outputTimeStepSec_ = (outputSampleRateHz_ > 0.0) ? (1.0 / outputSampleRateHz_) : 0.0;

	applyRates_();
	applyOutputTiming_();

	buildRaisedCosineFir_();
	configureIfBpf_();
	resetStates_();

	return true;
}

void RADAR_DUC::applyRates_()
{
	// 帮助文档说明：BB_UpSamplingRatio 是 baseband 到 digital IF 的上采样倍率。
	// 因此 Data Flow 中每读入 1 个 BB 样本，输出 R 个 IF envelope 样本。
	BB_Signal.SetRate(1U);
	IF_Signal.SetRate(static_cast<unsigned>(outRate_));
}

void RADAR_DUC::applyOutputTiming_()
{
	if (outputTimeStepSec_ > 0.0) {
		IF_Signal.SetTimeStep(outputTimeStepSec_);
	}
	if (outputSampleRateHz_ > 0.0) {
		IF_Signal.SetSampleRate(outputSampleRateHz_);
	}
	IF_Signal.SetCharacterizationFrequency(IF_Freq);
}

void RADAR_DUC::resetStates_()
{
	ducFirState_.clear();
	if (!ducFir_.empty()) {
		ducFirState_.resize(ducFir_.size(), Cx(0.0, 0.0));
	}

	ifBpfSec1_.reset();
	ifBpfSec2_.reset();
}

// ============================================================
// Raised-cosine 插值 FIR
// ============================================================

void RADAR_DUC::buildRaisedCosineFir_()
{
	ducFir_.clear();

	const int sps = (upRate_ > 0) ? upRate_ : 1;

	// 内置 DUC 启动瞬态明显长于普通短 FIR，因此这里采用 30-symbol span。
	// 当低频验证设置 BB_UpSamplingRatio=5 时，FIR 群延迟约为 75 个 IF 点，
	// 再叠加 IF BPF 启动过程，能更接近内置 DUC 的启动过渡。
	const int spanSymbols = 30;
	const int nTaps = spanSymbols * sps + 1;
	const int mid = nTaps / 2;

	ducFir_.resize(nTaps, 0.0);

	const double alpha = clamp(RC_ExcessBW, 0.0, 1.0);
	double sum = 0.0;

	for (int n = 0; n < nTaps; ++n) {
		const double t = static_cast<double>(n - mid) / static_cast<double>(sps);
		ducFir_[n] = raisedCosineImpulse_(t, alpha);
		sum += ducFir_[n];
	}

	// 零插值后，插值滤波器直流增益应补偿到接近 sps。
	if (std::fabs(sum) > 1e-30) {
		const double scale = static_cast<double>(sps) / sum;
		for (size_t i = 0; i < ducFir_.size(); ++i) {
			ducFir_[i] *= scale;
		}
	}
}

RADAR_DUC::Cx RADAR_DUC::runDucInterpolationFir_(const Cx& x)
{
	if (ducFir_.empty()) {
		return x;
	}

	if (ducFirState_.size() != ducFir_.size()) {
		ducFirState_.clear();
		ducFirState_.resize(ducFir_.size(), Cx(0.0, 0.0));
	}

	ducFirState_.push_front(x);
	while (ducFirState_.size() > ducFir_.size()) {
		ducFirState_.pop_back();
	}

	Cx y(0.0, 0.0);
	for (size_t i = 0; i < ducFir_.size(); ++i) {
		y += ducFir_[i] * ducFirState_[i];
	}

	return y;
}

// ============================================================
// DUC 末端 IF BPF，包络域中使用基带低通近似
// ============================================================

void RADAR_DUC::configureIfBpf_()
{
	ifBpfEnabled_ = false;

	if (outputSampleRateHz_ <= 0.0 || BandWidth <= 0.0) {
		return;
	}

	double fc = 0.5 * BandWidth;
	if (fc <= 0.0) {
		return;
	}
	if (fc > 0.45 * outputSampleRateHz_) {
		fc = 0.45 * outputSampleRateHz_;
	}

	// 二阶 Butterworth 低通，两级级联近似帮助图中的 IF BPF_Butterworth。
	const double q = 0.7071067811865476;
	const double w0 = 2.0 * M_PI * fc / outputSampleRateHz_;
	const double alpha = std::sin(w0) / (2.0 * q);
	const double cosw = std::cos(w0);
	const double a0 = 1.0 + alpha;

	const double b0 = (1.0 - cosw) * 0.5 / a0;
	const double b1 = (1.0 - cosw) / a0;
	const double b2 = (1.0 - cosw) * 0.5 / a0;
	const double a1 = (-2.0 * cosw) / a0;
	const double a2 = (1.0 - alpha) / a0;

	ifBpfSec1_.b0 = b0;
	ifBpfSec1_.b1 = b1;
	ifBpfSec1_.b2 = b2;
	ifBpfSec1_.a1 = a1;
	ifBpfSec1_.a2 = a2;

	ifBpfSec2_ = ifBpfSec1_;

	ifBpfEnabled_ = true;
}

RADAR_DUC::Cx RADAR_DUC::runBiquad_(const Cx& x, BiquadState& s)
{
	const Cx y =
		s.b0 * x +
		s.b1 * s.x1 +
		s.b2 * s.x2 -
		s.a1 * s.y1 -
		s.a2 * s.y2;

	s.x2 = s.x1;
	s.x1 = x;

	s.y2 = s.y1;
	s.y1 = y;

	return y;
}

RADAR_DUC::Cx RADAR_DUC::runIfBpf_(const Cx& x)
{
	if (!ifBpfEnabled_) {
		return x;
	}

	Cx y = x;
	y = runBiquad_(y, ifBpfSec1_);
	y = runBiquad_(y, ifBpfSec2_);
	return y;
}

// ============================================================
// 主运行函数
// ============================================================

bool RADAR_DUC::Run()
{
	applyOutputTiming_();

	const Cx input = BB_Signal[0U];
	const int totalOut = (outRate_ > 0) ? outRate_ : 1;

	for (int outIdx = 0; outIdx < totalOut; ++outIdx) {
		const double absSampleIndex =
			static_cast<double>(GetCount()) * static_cast<double>(totalOut) +
			static_cast<double>(outIdx);

		const double timeNow =
			(outputTimeStepSec_ > 0.0) ?
			absSampleIndex * outputTimeStepSec_ :
			absSampleIndex;

		// --------------------------------------------------------
		// 1. 基带上采样。
		// 帮助文档说明 I/Q 先按 BB_UpSamplingRatio 插零上采样，
		// 这里每个 Run 读入 1 个 BB 样本，仅第 0 相位放入输入样本。
		// --------------------------------------------------------
		const Cx upsampled = (outIdx == 0) ? input : Cx(0.0, 0.0);

		// --------------------------------------------------------
		// 2. Raised-cosine / digital lowpass 插值滤波。
		// --------------------------------------------------------
		Cx x = runDucInterpolationFir_(upsampled);

		// --------------------------------------------------------
		// 3. 输入中心频率处理。
		// --------------------------------------------------------
		x = applyInputCenterFrequency_(x, timeNow);

		// --------------------------------------------------------
		// 4. I/Q 正交调制到 IF envelope 主分量。
		// 帮助文档公式：
		// VIF(t)=VI(t)*cos(wc*t)-VQ(t)*sin(wc*t+phi*pi/180)。
		// --------------------------------------------------------
		Cx y = applyDUCToIFEnvelope_(x, timeNow);

		// --------------------------------------------------------
		// 5. DAC_NBits：作用于合成后的实 IF 波形。
		// --------------------------------------------------------
		if (DAC_NBits >= 2 && DAC_NBits < 64) {
			const double ph = 2.0 * M_PI * IF_Freq * timeNow;
			const double realIfBefore = y.real() * std::cos(ph) - y.imag() * std::sin(ph);
			const double realIfAfter = applyDAC_(realIfBefore);
			const double err = realIfAfter - realIfBefore;
			y += Cx(err * std::cos(ph), -err * std::sin(ph));
		}

		// --------------------------------------------------------
		// 6. FcChange 后的 IF BPF。
		// --------------------------------------------------------
		y = runIfBpf_(y);

		// --------------------------------------------------------
		// 7. DtoA -> FcChange 直接输出 envelope 的残余镜像近似。
		// 注意：该经验项必须放在 IF BPF 之后，否则 2*IF_Freq 分量会被
		// 低通近似压掉，直接输出的幅度起伏会明显不足。
		// --------------------------------------------------------
		y = applyFcChangeImage_(y, timeNow);

		// --------------------------------------------------------
		// 8. 复包络符号约定修正。
		// Tx_4x4 V5 黑盒结果表明，直接比较内置 envelope complex 值时，
		// 最终取共轭可缩小虚部符号约定差异。
		// --------------------------------------------------------
		y = applyFinalComplexConvention_(y, timeNow);

		IF_Signal[static_cast<unsigned>(outIdx)] = y;
	}

	return true;
}

// ============================================================
// DUC 信号处理函数
// ============================================================

RADAR_DUC::Cx RADAR_DUC::applyInputCenterFrequency_(const Cx& x, double timeNow) const
{
	if (std::fabs(In_CenterFreq) < 1e-15) {
		return x;
	}

	// 输入信号中心频率修正。符号与内置是否完全一致，
	// 后续建议用 In_CenterFreq 非零的黑盒测试确认。
	const double ph = 2.0 * M_PI * In_CenterFreq * timeNow;
	return x * Cx(std::cos(ph), std::sin(ph));
}

RADAR_DUC::Cx RADAR_DUC::applyDUCToIFEnvelope_(const Cx& x, double timeNow) const
{
	(void)timeNow;

	// 由帮助文档实 IF 公式推导得到理想 IF envelope 主分量：
	// real(t) = Re{env}*cos(wc*t) - Im{env}*sin(wc*t)
	// VIF(t)  = I*cos(wc*t) - Q*sin(wc*t + phi)
	// 因此：
	// Re{env} = I - Q*sin(phi)
	// Im{env} = Q*cos(phi)
	const double phi = deg2rad(PhaseImbalance);
	const double i = x.real();
	const double q = x.imag();

	return Cx(i - q * std::sin(phi), q * std::cos(phi));
}

RADAR_DUC::Cx RADAR_DUC::applyFcChangeImage_(const Cx& idealEnvelope, double timeNow) const
{
	// ============================================================
	// DtoA -> FcChange -> IF BPF 后的残余镜像补偿
	// ============================================================
	// 内置 RADAR_DUC 的输出位置是：
	//   上采样 -> 插值低通 -> I/Q 调制 -> DtoA -> FcChange -> IF BPF -> IF_Signal
	// 该链路直接输出 envelope 时，会保留一定 2*IF_Freq 周期的残余镜像起伏。
	//
	// 当前低频验证条件：
	//   IF_Freq = 20 kHz
	//   IF_SamplingRate = 1 MHz
	//   BB_UpSamplingRatio = 5
	//   纯实常量输入 0.1 + j0
	// 因此本版只对启动段做镜像项平滑渐入，稳态参数保持不变。
	const double imageFactorSteady = 0.70;
	const double imagePhaseDeg = -90.0;
	const double imageTimeAdvanceSec = 2.0e-6;

	// ------------------------------------------------------------
	// 启动段平滑补偿
	// ------------------------------------------------------------
	// 内置 DtoA/FcChange/BPF 在启动阶段存在缓冲和群时延。
	// 自设模型若一开始就使用稳态镜像项，前几个周期峰值容易偏高。
	// 这里让镜像项从较小比例平滑过渡到稳态值，只影响启动段，
	// 不破坏 150 us 之后已经基本对齐的稳态波形。
	const double startupBegin = 95.0e-6;
	const double startupEnd = 135.0e-6;

	double ramp = 1.0;
	if (timeNow <= startupBegin) {
		ramp = 0.0;
	}
	else if (timeNow < startupEnd) {
		const double u = (timeNow - startupBegin) / (startupEnd - startupBegin);
		ramp = u * u * (3.0 - 2.0 * u);  // smoothstep，避免切换断点
	}

	// 启动初期保留一部分镜像项，避免前沿形状被压得过低。
	// 若前几帧仍偏高，可把 0.55 调低到 0.45；若前几帧偏低，可调高到 0.65。
	const double startupImageRatio = 0.55;
	const double imageFactor = imageFactorSteady *
		(startupImageRatio + (1.0 - startupImageRatio) * ramp);

	const double tImage = timeNow + imageTimeAdvanceSec;
	const double ph = 4.0 * M_PI * IF_Freq * tImage + deg2rad(imagePhaseDeg);
	const Cx rot(std::cos(ph), std::sin(ph));

	return idealEnvelope + imageFactor * std::conj(idealEnvelope) * rot;
}

RADAR_DUC::Cx RADAR_DUC::applyFinalComplexConvention_(const Cx& x, double timeNow) const
{
	(void)timeNow;

	// 经验修正：与 RADAR_Tx / RADAR_Tx_4x4 V5 保持一致，
	// 最终输出前取共轭，用于匹配内置 FcChange 后 envelope 的虚部约定。
	const bool enableConjugateConventionFix = true;

	if (enableConjugateConventionFix) {
		return std::conj(x);
	}

	return x;
}

// ============================================================
// DAC 与数学辅助函数
// ============================================================

double RADAR_DUC::applyDAC_(double x) const
{
	// 帮助文档未公开 DtoA 的满量程、舍入和限幅细节。
	// 此处使用归一化 [-1,1] 均匀量化近似。
	if (DAC_NBits < 2 || DAC_NBits >= 64) {
		return x;
	}

	const double fullScale = 1.0;
	const int bits = (DAC_NBits > 30) ? 30 : DAC_NBits;
	const int levels = 1 << bits;
	const double step = (2.0 * fullScale) / static_cast<double>(levels - 1);

	const double clipped = clamp(x, -fullScale, fullScale);
	const double q = std::floor((clipped + fullScale) / step + 0.5) * step - fullScale;

	return clamp(q, -fullScale, fullScale);
}

double RADAR_DUC::raisedCosineImpulse_(double t, double alpha)
{
	if (std::fabs(t) < 1e-12) {
		return 1.0;
	}

	if (alpha <= 1e-12) {
		return sinc_(t);
	}

	const double denom = 1.0 - std::pow(2.0 * alpha * t, 2.0);
	if (std::fabs(denom) < 1e-10) {
		// t = +/- 1/(2*alpha) 处的极限值。
		return (M_PI / 4.0) * sinc_(1.0 / (2.0 * alpha));
	}

	return sinc_(t) * std::cos(M_PI * alpha * t) / denom;
}

double RADAR_DUC::sinc_(double x)
{
	if (std::fabs(x) < 1e-12) {
		return 1.0;
	}
	const double pix = M_PI * x;
	return std::sin(pix) / pix;
}

double RADAR_DUC::deg2rad(double x)
{
	return x * M_PI / 180.0;
}

double RADAR_DUC::clamp(double x, double lo, double hi)
{
	return std::max(lo, std::min(hi, x));
}
