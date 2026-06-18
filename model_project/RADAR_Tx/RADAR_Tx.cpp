#include "RADAR_Tx.h"

#include <cmath>
#include <algorithm>
#include <limits>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(RADAR_Tx)
{
	SET_MODEL_DESCRIPTION("RADAR Transmitter Front End");
	SET_MODEL_CATEGORY("Transmitter");

	// ============================================================
	// Ports
	// ============================================================
	{
		auto p = ADD_MODEL_INPUT(BB_Signal);
		p.SetDescription("BB signal");
	}

	{
		auto p = ADD_MODEL_OUTPUT(RF_Signal);
		p.SetDescription("RF signal");
	}

	// ============================================================
	// Basic transmitter parameters
	// ============================================================
	{
		auto p = ADD_MODEL_PARAM(TStep);
		p.SetUnit(SystemVueModelBuilder::Units::TIME);
		p.SetDefaultValue("0");
		p.SetDescription("simulation time step; TStep=0 results in use of externally set TStep ([0:&infin;))");
	}

	{
		auto p = ADD_MODEL_PARAM(RF_Freq);
		p.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		p.SetDefaultValue("1000000000");
		p.SetDescription("RF carrier frequency ((0:&infin;))");
	}

	{
		auto p = ADD_MODEL_PARAM(RF_Gain);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("1");
		p.SetDescription("Complex voltage gain; with form re+j*im; to specify gain in dB use dbpolar (dB, degree) ((-&infin;:&infin;))");
	}

	{
		auto p = ADD_MODEL_PARAM(IF_Freq);
		p.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		p.SetDefaultValue("20000000");
		p.SetDescription("IF carrier frequency ((0:&infin;))");
	}

	{
		auto p = ADD_MODEL_PARAM(IF_Gain);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("1");
		p.SetDescription("Complex voltage gain; with form re+j*im; to specify gain in dB use dbpolar (dB, degree) ((-&infin;:&infin;))");
	}

	{
		auto p = ADD_MODEL_PARAM(IF_SamplingRate);
		p.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		p.SetDefaultValue("50000000");
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
		p.SetDefaultValue("5");
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

	{
		auto p = ADD_MODEL_PARAM(DAC_UpSamplingRatio);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("1");
		p.SetDescription("the upsampling ratio of DAC from digital IF to analog IF ([1:&infin;))");
	}

	{
		auto p = ADD_MODEL_PARAM(NoiseFigure_RF_Gain);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("0");
		p.SetDescription("Input noise figure in dB for amplifier in RF");
	}

	{
		auto p = ADD_MODEL_PARAM(NoiseFigure_IF_Gain);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("0");
		p.SetDescription("Input noise figure in dB for amplifier in IF");
	}

	{
		auto p = ADD_MODEL_PARAM(NoiseFigure_Mixer);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("0");
		p.SetDescription("Double sideband noise figure in dB for Mixer RF");
	}

	// ============================================================
	// RF gain compression parameters
	// ============================================================
	{
		auto p = ADD_MODEL_ENUM_PARAM(GCType_RF_Gain, SelectedGCType);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.AddEnumeration("none", none);
		p.AddEnumeration("TOI", TOI);
		p.AddEnumeration("dBc1", dBc1);
		p.AddEnumeration("TOI+dBc1", TOI_dBc1);
		p.AddEnumeration("PSat+GCSat+TOI", PSat_GCSat_TOI);
		p.AddEnumeration("PSat+GCSat+dBc1", PSat_GCSat_dBc1);
		p.AddEnumeration("PSat+GCSat+TOI+dBc1", PSat_GCSat_TOI_dBc1);
		p.AddEnumeration("RappNonlinearity", RappNonlinearity);
		p.AddEnumeration("Gain compression vs input power", Gain_compression_vs_input_power);
		p.AddEnumeration("AM/AM and AM/PM vs input power", AM_AM_and_AMPM_vs_input_power);
		p.SetDefaultValue("none");
		p.SetDescription("Gain compression type for amplifier in RF");
	}

	{
		auto p = ADD_MODEL_PARAM(TOIout_RF_Gain);
		p.SetUnit(SystemVueModelBuilder::Units::POWER);
		p.SetDefaultValue("0.1");
		p.SetDescription("Output third order intercept power for amplifier in RF");
	}

	{
		auto p = ADD_MODEL_PARAM(dBc1out_RF_Gain);
		p.SetUnit(SystemVueModelBuilder::Units::POWER);
		p.SetDefaultValue("0.01");
		p.SetDescription("Output 1 dB gain compression power for amplifier in RF");
	}

	{
		auto p = ADD_MODEL_PARAM(PSat_RF_Gain);
		p.SetUnit(SystemVueModelBuilder::Units::POWER);
		p.SetDefaultValue("0.032");
		p.SetDescription("Saturation power for amplifier in RF");
	}

	{
		auto p = ADD_MODEL_PARAM(GCSat_RF_Gain);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("3");
		p.SetDescription("Gain compression at saturation in dB for amplifier in RF");
	}

	{
		auto p = ADD_MODEL_PARAM(RappS_RF_Gain);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("3");
		p.SetDescription("Rapp nonlinearity smoothness factor for amplifier in RF");
	}

	{
		auto p = ADD_MODEL_ARRAY_PARAM(GComp_RF_Gain, GComp_RF_Gain_Size);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("[0, 0, 0]");
		p.SetDescription("Array of triple values for Input Power(dBm) and either Gain(dB)/Phase(deg) change from small signal or AM-to-AM(dB/dB)/AM-to-PM(deg/dB) for amplifier in RF");
	}

	// ============================================================
	// IF gain compression parameters
	// ============================================================
	{
		auto p = ADD_MODEL_ENUM_PARAM(GCType_IF_Gain, SelectedGCType);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.AddEnumeration("none", none);
		p.AddEnumeration("TOI", TOI);
		p.AddEnumeration("dBc1", dBc1);
		p.AddEnumeration("TOI+dBc1", TOI_dBc1);
		p.AddEnumeration("PSat+GCSat+TOI", PSat_GCSat_TOI);
		p.AddEnumeration("PSat+GCSat+dBc1", PSat_GCSat_dBc1);
		p.AddEnumeration("PSat+GCSat+TOI+dBc1", PSat_GCSat_TOI_dBc1);
		p.AddEnumeration("RappNonlinearity", RappNonlinearity);
		p.AddEnumeration("Gain compression vs input power", Gain_compression_vs_input_power);
		p.AddEnumeration("AM/AM and AM/PM vs input power", AM_AM_and_AMPM_vs_input_power);
		p.SetDefaultValue("none");
		p.SetDescription("Gain compression type for amplifier in IF");
	}

	{
		auto p = ADD_MODEL_PARAM(TOIout_IF_Gain);
		p.SetUnit(SystemVueModelBuilder::Units::POWER);
		p.SetDefaultValue("0.1");
		p.SetDescription("Output third order intercept power for amplifier in IF");
	}

	{
		auto p = ADD_MODEL_PARAM(dBc1out_IF_Gain);
		p.SetUnit(SystemVueModelBuilder::Units::POWER);
		p.SetDefaultValue("0.01");
		p.SetDescription("Output 1 dB gain compression power for amplifier in IF");
	}

	{
		auto p = ADD_MODEL_PARAM(PSat_IF_Gain);
		p.SetUnit(SystemVueModelBuilder::Units::POWER);
		p.SetDefaultValue("0.032");
		p.SetDescription("Saturation power for amplifier in IF");
	}

	{
		auto p = ADD_MODEL_PARAM(GCSat_IF_Gain);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("3");
		p.SetDescription("Gain compression at saturation in dB for amplifier in IF");
	}

	{
		auto p = ADD_MODEL_PARAM(RappS_IF_Gain);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("3");
		p.SetDescription("Rapp nonlinearity smoothness factor for amplifier in IF");
	}

	{
		auto p = ADD_MODEL_ARRAY_PARAM(GComp_IF_Gain, GComp_IF_Gain_Size);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("[0, 0, 0]");
		p.SetDescription("Array of triple values for Input Power(dBm) and either Gain(dB)/Phase(deg) change from small signal or AM-to-AM(dB/dB)/AM-to-PM(deg/dB) for amplifier in IF");
	}

	return true;
}
#endif

// ============================================================
// 状态结构构造与复位
// ============================================================

RADAR_Tx::BiquadState::BiquadState()
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

void RADAR_Tx::BiquadState::reset()
{
	x1 = Cx(0.0, 0.0);
	x2 = Cx(0.0, 0.0);
	y1 = Cx(0.0, 0.0);
	y2 = Cx(0.0, 0.0);
}

// ============================================================
// 构造函数
// ============================================================

RADAR_Tx::RADAR_Tx()
	: TStep(0.0)
	, RF_Freq(1000000000.0)
	, RF_Gain(1.0, 0.0)
	, IF_Freq(20000000.0)
	, IF_Gain(1.0, 0.0)
	, IF_SamplingRate(50000000.0)
	, BandWidth(5000000.0)
	, In_CenterFreq(0.0)
	, BB_UpSamplingRatio(5)
	, RC_ExcessBW(0.22)
	, PhaseImbalance(0.0)
	, DAC_NBits(8)
	, DAC_UpSamplingRatio(1)
	, NoiseFigure_RF_Gain(0.0)
	, NoiseFigure_IF_Gain(0.0)
	, NoiseFigure_Mixer(0.0)
	, GCType_RF_Gain(none)
	, TOIout_RF_Gain(0.1)
	, dBc1out_RF_Gain(0.01)
	, PSat_RF_Gain(0.032)
	, GCSat_RF_Gain(3.0)
	, RappS_RF_Gain(3)
	, GComp_RF_Gain(0)
	, GComp_RF_Gain_Size(0)
	, GCType_IF_Gain(none)
	, TOIout_IF_Gain(0.1)
	, dBc1out_IF_Gain(0.01)
	, PSat_IF_Gain(0.032)
	, GCSat_IF_Gain(3.0)
	, RappS_IF_Gain(3)
	, GComp_IF_Gain(0)
	, GComp_IF_Gain_Size(0)
	, sampleRateHz_(0.0)
	, timeStepSec_(0.0)
	, outputSampleRateHz_(0.0)
	, outputTimeStepSec_(0.0)
	, bbUp_(5)
	, dacUp_(1)
	, outRate_(5)
	, noisePrepared_(false)
	, noiseSigmaRF_(0.0)
	, noiseSigmaIF_(0.0)
	, noiseSigmaMixer_(0.0)
	, seedRF_(0x13579BDFu)
	, seedIF_(0x2468ACE0u)
	, seedMixer_(0x10203040u)
	, ifBpfEnabled_(false)
	, rfBpfEnabled_(false)
{
}

// ============================================================
// Setup 与载频传播
// ============================================================

ERESULT RADAR_Tx::PropagateCharacterizationFrequency()
{
	RF_Signal.SetCharacterizationFrequency(RF_Freq);
	return true;
}

bool RADAR_Tx::Setup()
{
	bbUp_ = (BB_UpSamplingRatio > 0) ? BB_UpSamplingRatio : 1;
	dacUp_ = (DAC_UpSamplingRatio > 0) ? DAC_UpSamplingRatio : 1;
	outRate_ = bbUp_ * dacUp_;
	if (outRate_ < 1) {
		outRate_ = 1;
	}

	sampleRateHz_ = IF_SamplingRate;
	if (TStep > 0.0) {
		timeStepSec_ = TStep;
		sampleRateHz_ = 1.0 / TStep;
	}
	else if (sampleRateHz_ > 0.0) {
		timeStepSec_ = 1.0 / sampleRateHz_;
	}
	else {
		sampleRateHz_ = 0.0;
		timeStepSec_ = 0.0;
	}

	outputSampleRateHz_ = sampleRateHz_ * static_cast<double>(dacUp_);
	if (outputSampleRateHz_ > 0.0) {
		outputTimeStepSec_ = 1.0 / outputSampleRateHz_;
	}
	else {
		outputTimeStepSec_ = 0.0;
	}

	applyRates_();

	RF_Signal.SetCharacterizationFrequency(RF_Freq);

	noisePrepared_ = false;
	seedRF_ = 0x13579BDFu;
	seedIF_ = 0x2468ACE0u;
	seedMixer_ = 0x10203040u;

	if (!prepareTables()) {
		return false;
	}

	buildRaisedCosineFir_();
	configureIfBpf_();
	configureRfBpf_();
	resetStates_();
	applyOutputTiming_();

	return true;
}

void RADAR_Tx::applyRates_()
{
	BB_Signal.SetRate(1U);
	RF_Signal.SetRate(static_cast<unsigned>(outRate_));
}

void RADAR_Tx::applyOutputTiming_()
{
	if (outputTimeStepSec_ > 0.0) {
		RF_Signal.SetTimeStep(outputTimeStepSec_);
	}
	if (outputSampleRateHz_ > 0.0) {
		RF_Signal.SetSampleRate(outputSampleRateHz_);
	}
	RF_Signal.SetCharacterizationFrequency(RF_Freq);
}

void RADAR_Tx::resetStates_()
{
	ducFirState_.clear();
	ducFirState_.resize(ducFir_.size(), Cx(0.0, 0.0));

	ifBpfSec1_.reset();
	ifBpfSec2_.reset();
	rfBpfSec1_.reset();
	rfBpfSec2_.reset();
}

// ============================================================
// 表格参数与噪声准备
// ============================================================

bool RADAR_Tx::prepareTables()
{
	rfTable_ = GCompTable();
	ifTable_ = GCompTable();

	if (GCType_RF_Gain == Gain_compression_vs_input_power ||
		GCType_RF_Gain == AM_AM_and_AMPM_vs_input_power) {
		parseGCompArray(GComp_RF_Gain, static_cast<int>(GComp_RF_Gain_Size), rfTable_);
	}

	if (GCType_IF_Gain == Gain_compression_vs_input_power ||
		GCType_IF_Gain == AM_AM_and_AMPM_vs_input_power) {
		parseGCompArray(GComp_IF_Gain, static_cast<int>(GComp_IF_Gain_Size), ifTable_);
	}

	return true;
}

bool RADAR_Tx::parseGCompArray(const double* data,
	int size,
	GCompTable& table) const
{
	table = GCompTable();

	if (data == 0 || size < 9 || (size % 3) != 0) {
		return false;
	}

	const int n = size / 3;

	table.pinDbm.resize(n);
	table.gainChangeDb.resize(n);
	table.phaseChangeDeg.resize(n);

	for (int i = 0; i < n; ++i) {
		table.pinDbm[i] = data[3 * i + 0];
		table.gainChangeDb[i] = data[3 * i + 1];
		table.phaseChangeDeg[i] = data[3 * i + 2];
	}

	for (int i = 1; i < n; ++i) {
		if (table.pinDbm[i] <= table.pinDbm[i - 1]) {
			table = GCompTable();
			return false;
		}
	}

	table.valid = true;
	return true;
}

bool RADAR_Tx::prepareNoise()
{
	if (noisePrepared_) {
		return true;
	}

	noisePrepared_ = true;

	noiseSigmaRF_ = 0.0;
	noiseSigmaIF_ = 0.0;
	noiseSigmaMixer_ = 0.0;

	if (outputSampleRateHz_ <= 0.0) {
		return true;
	}

	const double kBoltz = 1.38064852e-23;
	const double t0 = 290.0;
	const double refR = 50.0;
	const double fs = outputSampleRateHz_;

	auto calcSigma = [=](double nfDb) -> double {
		if (nfDb <= 0.0) {
			return 0.0;
		}

		const double nfLin = std::pow(10.0, nfDb / 10.0);
		if (nfLin <= 1.0) {
			return 0.0;
		}

		return std::sqrt(kBoltz * t0 * (nfLin - 1.0) * fs * refR);
	};

	noiseSigmaRF_ = calcSigma(NoiseFigure_RF_Gain);
	noiseSigmaIF_ = calcSigma(NoiseFigure_IF_Gain);
	noiseSigmaMixer_ = calcSigma(NoiseFigure_Mixer);

	return true;
}

// ============================================================
// DUC 插值与滤波器
// ============================================================

void RADAR_Tx::buildRaisedCosineFir_()
{
	ducFir_.clear();

	const int sps = (bbUp_ > 0) ? bbUp_ : 1;

	// 内置 RADAR_DUC 的启动瞬态明显长于普通 6-symbol 插值滤波器。
	// 在低频验证工况（BB_UpSamplingRatio=5，TStep=1us）下，
	// 内置输出约在 100us 附近开始进入明显起振区。
	// 30-symbol raised-cosine FIR 的群时延约为：
	//     30*sps/2 = 15*sps 个 IF 采样点
	// 当 sps=5 时约为 75 个 IF 点，再叠加后级 BPF 启动过程，
	// 与当前黑盒测试中观察到的 100us 左右起振更接近。
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

	// 零插值后需要把插值滤波器直流增益补偿到接近 sps。
	if (std::fabs(sum) > 1e-30) {
		const double scale = static_cast<double>(sps) / sum;
		for (size_t i = 0; i < ducFir_.size(); ++i) {
			ducFir_[i] *= scale;
		}
	}
}

RADAR_Tx::Cx RADAR_Tx::runDucInterpolationFir_(const Cx& x)
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

void RADAR_Tx::configureIfBpf_()
{
	ifBpfEnabled_ = false;

	// IF 包络域中的等效 BPF。由于这里使用复包络近似，
	// 按基带低通形式模拟 DUC 输出端的 IF 滤波效果。
	if (sampleRateHz_ <= 0.0 || BandWidth <= 0.0) {
		return;
	}

	double fc = 0.5 * BandWidth;
	if (fc <= 0.0) {
		return;
	}
	if (fc > 0.45 * sampleRateHz_) {
		fc = 0.45 * sampleRateHz_;
	}

	const double q = 0.7071067811865476;
	const double w0 = 2.0 * M_PI * fc / sampleRateHz_;
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

void RADAR_Tx::configureRfBpf_()
{
	rfBpfEnabled_ = false;

	// RF BPF 以 RF_Freq 为中心。在 RF 包络域中可等效为基带低通，
	// 因此这里采用与 IF BPF 类似的低通近似。
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

	rfBpfSec1_.b0 = b0;
	rfBpfSec1_.b1 = b1;
	rfBpfSec1_.b2 = b2;
	rfBpfSec1_.a1 = a1;
	rfBpfSec1_.a2 = a2;

	rfBpfSec2_ = rfBpfSec1_;

	rfBpfEnabled_ = true;
}

RADAR_Tx::Cx RADAR_Tx::runBiquad(const Cx& x,
	BiquadState& s)
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

RADAR_Tx::Cx RADAR_Tx::runIfBpf_(const Cx& x)
{
	if (!ifBpfEnabled_) {
		return x;
	}

	Cx y = x;
	y = runBiquad(y, ifBpfSec1_);
	y = runBiquad(y, ifBpfSec2_);
	return y;
}

RADAR_Tx::Cx RADAR_Tx::runRfBpf_(const Cx& x)
{
	if (!rfBpfEnabled_) {
		return x;
	}

	Cx y = x;
	y = runBiquad(y, rfBpfSec1_);
	y = runBiquad(y, rfBpfSec2_);
	return y;
}

// ============================================================
// 主运行函数
// ============================================================

bool RADAR_Tx::Run()
{
	if (!prepareNoise()) {
		return false;
	}

	applyOutputTiming_();
	RF_Signal.SetCharacterizationFrequency(RF_Freq);

	const Cx input = BB_Signal[0U];

	const int bbUp = (bbUp_ > 0) ? bbUp_ : 1;
	const int dacUp = (dacUp_ > 0) ? dacUp_ : 1;
	const int totalOut = (outRate_ > 0) ? outRate_ : 1;

	Cx xDucHold(0.0, 0.0);

	for (int outIdx = 0; outIdx < totalOut; ++outIdx) {
		const int bbPhase = outIdx / dacUp;
		const int dacPhase = outIdx % dacUp;

		const double absSampleIndex =
			static_cast<double>(GetCount()) * static_cast<double>(totalOut) +
			static_cast<double>(outIdx);

		const double timeNow =
			(outputTimeStepSec_ > 0.0) ?
			absSampleIndex * outputTimeStepSec_ :
			absSampleIndex;

		// --------------------------------------------------------
		// 1. RADAR_DUC：基带上采样 + raised-cosine 插值。
		// 上采样序列只在第一个相位放入输入样本，其余相位补零。
		// 当 DAC_UpSamplingRatio > 1 时，额外 DAC 相位暂按保持处理。
		// --------------------------------------------------------
		Cx upsampled(0.0, 0.0);
		if (dacPhase == 0) {
			upsampled = (bbPhase == 0) ? input : Cx(0.0, 0.0);
		}

		Cx xDuc(0.0, 0.0);
		if (dacPhase == 0) {
			xDuc = runDucInterpolationFir_(upsampled);
			xDucHold = xDuc;
		}
		else {
			// DAC 额外上采样相位采用简单保持近似。
			xDuc = xDucHold;
		}

		// I/Q 上变频前，先处理输入中心频率。
		xDuc = applyInputCenterFrequency_(xDuc, timeNow);

		// 根据 RADAR_DUC 帮助文档中的 I*cos - Q*sin 公式，
		// 先得到理想 IF 复包络主分量。残余镜像项不要放在这里，
		// 否则会被后面的 IF/RF 低通近似滤波器过度衰减。
		Cx xIf = applyDUCToIFEnvelope_(xDuc, timeNow);

		// DAC_NBits 在内置 DUC 中作用于实 IF 波形。
		// 这里先把 IF 包络还原成等效实 IF，对其实行简化量化，
		// 再把量化误差折回包络域。高位数验证时该项基本透明。
		if (DAC_NBits >= 2 && DAC_NBits < 64) {
			const double ph = 2.0 * M_PI * IF_Freq * timeNow;
			const double realIfBefore = xIf.real() * std::cos(ph) - xIf.imag() * std::sin(ph);
			const double realIfAfter = applyDAC_(realIfBefore);
			const double err = realIfAfter - realIfBefore;
			xIf += Cx(err * std::cos(ph), -err * std::sin(ph));
		}

		// DUC 内部 IF BPF 的等效滤波。
		xIf = runIfBpf_(xIf);

		// --------------------------------------------------------
		// 2. IF 放大器。
		// --------------------------------------------------------
		xIf = addNoise(xIf, noiseSigmaIF_, seedIF_);
		xIf = applyStage(xIf,
			IF_Gain,
			GCType_IF_Gain,
			TOIout_IF_Gain,
			dBc1out_IF_Gain,
			PSat_IF_Gain,
			GCSat_IF_Gain,
			RappS_IF_Gain,
			ifTable_);

		// --------------------------------------------------------
		// 3. RF Mixer：上边带，LO = RF_Freq - IF_Freq。
		// 复包络近似下主要体现为载频从 IF_Freq 转为 RF_Freq。
		// --------------------------------------------------------
		Cx xRf = addNoise(xIf, noiseSigmaMixer_, seedMixer_);
		xRf = applyMixerToRFEnvelope_(xRf, timeNow);

		// --------------------------------------------------------
		// 4. RF BPF 与 RF 放大器。
		// --------------------------------------------------------
		xRf = runRfBpf_(xRf);
		xRf = addNoise(xRf, noiseSigmaRF_, seedRF_);
		xRf = applyStage(xRf,
			RF_Gain,
			GCType_RF_Gain,
			TOIout_RF_Gain,
			dBc1out_RF_Gain,
			PSat_RF_Gain,
			GCSat_RF_Gain,
			RappS_RF_Gain,
			rfTable_);

		// --------------------------------------------------------
		// 5. FcChange / 实信号转包络残余镜像近似。
		// 关键修正：V2 把镜像项加在 IF BPF 之前，2*IF_Freq 分量会被
		// 后续低通近似滤波器压掉，所以 imageFactor 调大也很难产生
		// 内置模块那种明显起伏。这里把镜像残余放到最终 RF 包络输出
		// 之前，使直接观察 Tx 输出时能保留内置 DtoA->FcChange 后的
		// 2*IF_Freq 包络起伏。
		// --------------------------------------------------------
		xRf = applyFcChangeImage_(xRf, timeNow);

		// --------------------------------------------------------
		// 6. 最终复包络约定修正。
		// 按 RADAR_Tx_4x4 V5 的最优结果处理：在直接观察 RF envelope
		// complex 数据时，内置模块的虚部约定与自设主链路相反，
		// 因此输出前取共轭，以缩小 real/imag 逐点误差。
		// --------------------------------------------------------
		xRf = applyFinalComplexPhaseCorrection_(xRf, timeNow);

		RF_Signal[static_cast<unsigned>(outIdx)] = xRf;
	}

	return true;
}

// ============================================================
// 信号处理辅助函数
// ============================================================

RADAR_Tx::Cx RADAR_Tx::applyInputCenterFrequency_(const Cx& x,
	double timeNow) const
{
	if (std::fabs(In_CenterFreq) < 1e-15) {
		return x;
	}

	// 将带 In_CenterFreq 的输入搬移到 DUC 参考频率。
	// 非零 In_CenterFreq 的符号仍需要后续黑盒验证。
	const double ph = 2.0 * M_PI * In_CenterFreq * timeNow;
	return x * Cx(std::cos(ph), std::sin(ph));
}

RADAR_Tx::Cx RADAR_Tx::applyDUCToIFEnvelope_(const Cx& x,
	double timeNow) const
{
	(void)timeNow;

	// RADAR_DUC 帮助文档给出的实 IF 公式为：
	//   VIF(t) = VI(t)*cos(wc*t) - VQ(t)*sin(wc*t + phi*pi/180)
	// EnvelopeSignal 由包络还原实信号时等效为：
	//   real(t) = Re{env}*cos(wc*t) - Im{env}*sin(wc*t)
	// 因此理想 IF 复包络主分量应为：
	//   Re = I - Q*sin(phi), Im = Q*cos(phi)
	// 注意：这里不再加入 2*IF_Freq 镜像项。镜像项如果放在这里，
	// 会被后续 IF/RF BPF 的低通近似严重衰减，导致直接 Tx 输出
	// 起伏始终跟不上内置模块。
	const double phi = deg2rad(PhaseImbalance);
	const double i = x.real();
	const double q = x.imag();

	return Cx(i - q * std::sin(phi), q * std::cos(phi));
}

RADAR_Tx::Cx RADAR_Tx::applyFcChangeImage_(const Cx& idealEnvelope,
	double timeNow) const
{
	// 内置 DUC 的 DtoA 输出是实 IF 信号，之后通过 FcChange 转成
	// IF/RF envelope。直接观察该 envelope 时，实信号转包络过程会
	// 留下一个以 2*IF_Freq 变化的残余镜像/起伏项。
	//
	// 该残余镜像不能放在 IF/RF BPF 之前，否则会被后级低通近似压低，
	// 导致直接 Tx 输出起伏明显不足。因此这里在最终 RF envelope 输出前
	// 叠加经验镜像项，用来模拟内置 DtoA -> FcChange 后的 2*IF_Freq 起伏。
	//
	// 按 RADAR_Tx_4x4 V5 当前最优版本同步修正：
	//   imageFactor = 0.55
	//   imagePhaseDeg = -270 deg
	//   imageTimeAdvanceSec = 0
	// 这样既保留单通道 Tx 已经接近内置的幅度起伏，又避免额外时间提前量
	// 造成 real/imag 逐点超前。
	const double imageFactor = 0.55;
	const double imagePhaseDeg = -270.0;
	const double imageTimeAdvanceSec = 0.0;

	const double tImage = timeNow + imageTimeAdvanceSec;
	const double ph = 4.0 * M_PI * IF_Freq * tImage + deg2rad(imagePhaseDeg);
	const Cx rot(std::cos(ph), std::sin(ph));

	return idealEnvelope + imageFactor * std::conj(idealEnvelope) * rot;
}

RADAR_Tx::Cx RADAR_Tx::applyFinalComplexPhaseCorrection_(const Cx& x,
	double timeNow) const
{
	(void)timeNow;

	// RADAR_Tx_4x4 V4/V5 的直接 RF envelope 对比表明：
	// real(self) 与 real(sv) 基本同相后，imag(self) 与 imag(sv)
	// 主要表现为包络虚部约定相反。该差异来自 DtoA/FcChange/BPF
	// 链路对实信号转复包络时采用的解析包络符号约定。
	//
	// 因此这里不再做普通固定相位旋转，而是在最终输出前取共轭：
	//     y = conj(x)
	// 该处理保持实部波形基本不变，同时翻转虚部符号，可改善直接
	// 比较 RF_Signal complex 数据时的 re/im 逐点误差。
	//
	// 如果后续做 Tx -> Rx 闭环验证时发现接收结果反而变差，可将
	// enableConjugateConventionFix 改为 false，仅在直接比较 Tx 输出
	// envelope 时启用该经验修正。
	const bool enableConjugateConventionFix = true;

	if (enableConjugateConventionFix) {
		return std::conj(x);
	}

	return x;
}

RADAR_Tx::Cx RADAR_Tx::applyMixerToRFEnvelope_(const Cx& x,
	double timeNow) const
{
	(void)timeNow;

	// The built-in Tx mixer uses upper sideband and LO = RF_Freq - IF_Freq.
	// In the complex-envelope abstraction, this converts the IF envelope to an
	// RF envelope at RF_Freq without changing its baseband I/Q value.
	return x;
}

RADAR_Tx::Cx RADAR_Tx::addNoise(const Cx& x,
	double sigma,
	uint32_t& seed)
{
	if (sigma <= 0.0) {
		return x;
	}

	return x + Cx(sigma * randn_(seed), sigma * randn_(seed));
}

double RADAR_Tx::applyDAC_(double x) const
{
	// 帮助文档没有公开 DtoA 的满量程、舍入和限幅规则。
	// 这里采用归一化 DAC 近似；早期对齐建议使用较高 DAC_NBits
	// 或较小输入幅度，避免量化细节主导误差。
	if (DAC_NBits < 2 || DAC_NBits >= 64) {
		return x;
	}

	const double fullScale = 1.0;
	const int levels = 1 << ((DAC_NBits > 30) ? 30 : DAC_NBits);
	const double step = (2.0 * fullScale) / static_cast<double>(levels - 1);

	const double clipped = clamp(x, -fullScale, fullScale);
	const double q = std::floor((clipped + fullScale) / step + 0.5) * step - fullScale;

	return clamp(q, -fullScale, fullScale);
}

// ============================================================
// 增益与压缩非线性
// ============================================================

RADAR_Tx::Cx RADAR_Tx::applyStage(const Cx& x,
	const Cx& gain,
	SelectedGCType gcType,
	double toiOut,
	double dbc1Out,
	double psat,
	double gcSat,
	int rappS,
	const GCompTable& table) const
{
	const Cx yLinear = x * gain;

	if (gcType == none) {
		return yLinear;
	}

	const double aLin = std::abs(yLinear);
	if (aLin <= 0.0) {
		return Cx(0.0, 0.0);
	}

	const double gainAbs = std::abs(gain);
	const double ain = std::abs(x);

	if (gcType == Gain_compression_vs_input_power ||
		gcType == AM_AM_and_AMPM_vs_input_power) {
		return applyTableCompressionComplex(yLinear, ain, gainAbs, gcType, table);
	}

	const double aOut = applyCompressionMagnitude(ain,
		gainAbs,
		gcType,
		toiOut,
		dbc1Out,
		psat,
		gcSat,
		rappS,
		table);

	return yLinear * (aOut / aLin);
}

double RADAR_Tx::applyCompressionMagnitude(double ain,
	double gainAbs,
	SelectedGCType gcType,
	double toiOut,
	double dbc1Out,
	double psat,
	double gcSat,
	int rappS,
	const GCompTable& table) const
{
	if (ain <= 0.0 || gainAbs <= 0.0) {
		return 0.0;
	}

	switch (gcType) {
	case TOI:
		return applyTOI(ain, gainAbs, toiOut);

	case dBc1:
		return applydBc1(ain, gainAbs, dbc1Out);

	case TOI_dBc1:
		return applyTOIdBc1(ain, gainAbs, toiOut, dbc1Out);

	case PSat_GCSat_TOI:
	case PSat_GCSat_dBc1:
	case PSat_GCSat_TOI_dBc1:
		return applyPSat(ain, gainAbs, psat, gcSat);

	case RappNonlinearity:
		return applyRapp(ain, gainAbs, psat, rappS);

	case Gain_compression_vs_input_power:
	case AM_AM_and_AMPM_vs_input_power:
		return applyTableCompressionMagnitude(ain, gainAbs, table);

	default:
		return gainAbs * ain;
	}
}

RADAR_Tx::Cx RADAR_Tx::applyTableCompressionComplex(const Cx& yLinear,
	double ain,
	double gainAbs,
	SelectedGCType gcType,
	const GCompTable& table) const
{
	if (!table.valid || table.pinDbm.size() < 2 || ain <= 0.0) {
		return yLinear;
	}

	const double aLin = std::abs(yLinear);
	if (aLin <= 0.0) {
		return Cx(0.0, 0.0);
	}

	const double refR = 50.0;
	const double pinNow = peakVoltageToDbm(ain, refR);

	double gainDb = 0.0;
	double phaseDeg = 0.0;
	if (!lookupTable(pinNow, table, gainDb, phaseDeg)) {
		return yLinear;
	}

	Cx y = yLinear * dbToLinVoltage(gainDb);

	if (gcType == AM_AM_and_AMPM_vs_input_power) {
		const double ph = deg2rad(phaseDeg);
		y *= Cx(std::cos(ph), std::sin(ph));
	}
	else {
		(void)gainAbs;
	}

	return y;
}

bool RADAR_Tx::lookupTable(double pinDbm,
	const GCompTable& table,
	double& gainChangeDb,
	double& phaseChangeDeg) const
{
	gainChangeDb = 0.0;
	phaseChangeDeg = 0.0;

	if (!table.valid || table.pinDbm.size() < 1) {
		return false;
	}

	const int n = static_cast<int>(table.pinDbm.size());

	if (pinDbm <= table.pinDbm.front()) {
		gainChangeDb = table.gainChangeDb.front();
		phaseChangeDeg = table.phaseChangeDeg.front();
		return true;
	}

	if (pinDbm >= table.pinDbm.back()) {
		gainChangeDb = table.gainChangeDb.back();
		phaseChangeDeg = table.phaseChangeDeg.back();
		return true;
	}

	int k = 0;
	for (int i = 0; i < n - 1; ++i) {
		if (pinDbm >= table.pinDbm[i] && pinDbm <= table.pinDbm[i + 1]) {
			k = i;
			break;
		}
	}

	const double x0 = table.pinDbm[k];
	const double x1 = table.pinDbm[k + 1];
	const double t = (pinDbm - x0) / (x1 - x0);

	gainChangeDb = table.gainChangeDb[k] +
		t * (table.gainChangeDb[k + 1] - table.gainChangeDb[k]);

	phaseChangeDeg = table.phaseChangeDeg[k] +
		t * (table.phaseChangeDeg[k + 1] - table.phaseChangeDeg[k]);

	return true;
}

double RADAR_Tx::applyTOI(double ain,
	double gainAbs,
	double toiOut) const
{
	if (toiOut <= 0.0) {
		return gainAbs * ain;
	}

	const double refR = 50.0;
	const double toiV = wattToPeakVoltage(toiOut, refR);
	if (toiV <= 0.0) {
		return gainAbs * ain;
	}

	const double c1 = gainAbs;
	const double c3 = -(c1 * c1 * c1) / (toiV * toiV);

	const double xmax = std::sqrt(-c1 / (3.0 * c3));
	const double ymax = c1 * xmax + c3 * xmax * xmax * xmax;

	if (ain >= xmax) {
		return ymax;
	}

	double y = c1 * ain + c3 * ain * ain * ain;
	if (y < 0.0) {
		y = 0.0;
	}

	return y;
}

double RADAR_Tx::applydBc1(double ain,
	double gainAbs,
	double dbc1Out) const
{
	if (dbc1Out <= 0.0) {
		return gainAbs * ain;
	}

	const double refR = 50.0;
	const double p1V = wattToPeakVoltage(dbc1Out, refR);
	const double oneDbRatio = std::pow(10.0, -1.0 / 20.0);

	if (p1V <= 0.0 || gainAbs <= 0.0) {
		return gainAbs * ain;
	}

	const double c1 = gainAbs;
	const double x1 = p1V / (c1 * oneDbRatio);
	const double y1 = p1V;

	const double c3 = (y1 - c1 * x1) / (x1 * x1 * x1);

	const double xmax = std::sqrt(-c1 / (3.0 * c3));
	const double ymax = c1 * xmax + c3 * xmax * xmax * xmax;

	if (ain >= xmax) {
		return ymax;
	}

	double y = c1 * ain + c3 * ain * ain * ain;
	if (y < 0.0) {
		y = 0.0;
	}

	return y;
}

double RADAR_Tx::applyTOIdBc1(double ain,
	double gainAbs,
	double toiOut,
	double dbc1Out) const
{
	const double yToi = applyTOI(ain, gainAbs, toiOut);
	const double yP1 = applydBc1(ain, gainAbs, dbc1Out);
	return std::min(yToi, yP1);
}

double RADAR_Tx::applyPSat(double ain,
	double gainAbs,
	double psat,
	double gcSat) const
{
	if (psat <= 0.0) {
		return gainAbs * ain;
	}

	const double refR = 50.0;
	const double psatV = wattToPeakVoltage(psat, refR);

	if (psatV <= 0.0) {
		return gainAbs * ain;
	}

	const double yLinear = gainAbs * ain;
	(void)gcSat;

	const double y = psatV * std::tanh(yLinear / psatV);
	return std::min(y, psatV);
}

double RADAR_Tx::applyRapp(double ain,
	double gainAbs,
	double psat,
	int rappS) const
{
	if (psat <= 0.0) {
		return gainAbs * ain;
	}

	const double refR = 50.0;
	const double psatV = wattToPeakVoltage(psat, refR);

	if (psatV <= 0.0) {
		return gainAbs * ain;
	}

	double s = static_cast<double>((rappS > 0) ? rappS : 3);
	if (s < 0.5) {
		s = 0.5;
	}

	const double yLinear = gainAbs * ain;
	const double ratio = yLinear / psatV;

	const double denom =
		std::pow(1.0 + std::pow(ratio, 2.0 * s), 1.0 / (2.0 * s));

	return yLinear / denom;
}

double RADAR_Tx::applyTableCompressionMagnitude(double ain,
	double gainAbs,
	const GCompTable& table) const
{
	if (!table.valid || table.pinDbm.size() < 2) {
		return gainAbs * ain;
	}

	const double refR = 50.0;
	const double pinNow = peakVoltageToDbm(ain, refR);

	double gainDb = 0.0;
	double phaseDeg = 0.0;
	if (!lookupTable(pinNow, table, gainDb, phaseDeg)) {
		return gainAbs * ain;
	}

	return gainAbs * ain * dbToLinVoltage(gainDb);
}

// ============================================================
// 随机数与插值脉冲辅助函数
// ============================================================

double RADAR_Tx::randUniform_(uint32_t& seed) const
{
	seed = 1664525U * seed + 1013904223U;
	return (static_cast<double>(seed) + 0.5) / 4294967296.0;
}

double RADAR_Tx::randn_(uint32_t& seed) const
{
	double s = 0.0;
	for (int i = 0; i < 12; ++i) {
		s += randUniform_(seed);
	}
	return s - 6.0;
}

double RADAR_Tx::sinc_(double x)
{
	if (std::fabs(x) < 1e-12) {
		return 1.0;
	}
	return std::sin(M_PI * x) / (M_PI * x);
}

double RADAR_Tx::raisedCosineImpulse_(double t,
	double alpha)
{
	if (alpha <= 1e-12) {
		return sinc_(t);
	}

	const double den = 1.0 - 4.0 * alpha * alpha * t * t;
	if (std::fabs(den) < 1e-10) {
		// Limit at t = +/- 1/(2*alpha)
		return 0.5 * alpha * std::sin(M_PI / (2.0 * alpha));
	}

	return sinc_(t) * std::cos(M_PI * alpha * t) / den;
}

// ============================================================
// 数学辅助函数
// ============================================================

double RADAR_Tx::dbToLinVoltage(double db)
{
	return std::pow(10.0, db / 20.0);
}

double RADAR_Tx::linToDbVoltage(double lin)
{
	if (lin <= 0.0) {
		lin = 1e-300;
	}

	return 20.0 * std::log10(lin);
}

double RADAR_Tx::wattToDbm(double w)
{
	if (w <= 0.0) {
		w = 1e-300;
	}

	return 10.0 * std::log10(w) + 30.0;
}

double RADAR_Tx::dbmToWatt(double dbm)
{
	return std::pow(10.0, (dbm - 30.0) / 10.0);
}

double RADAR_Tx::wattToPeakVoltage(double w, double r)
{
	if (w <= 0.0 || r <= 0.0) {
		return 0.0;
	}

	return std::sqrt(2.0 * r * w);
}

double RADAR_Tx::peakVoltageToWatt(double v, double r)
{
	if (r <= 0.0) {
		return 0.0;
	}

	return (v * v) / (2.0 * r);
}

double RADAR_Tx::peakVoltageToDbm(double v, double r)
{
	return wattToDbm(peakVoltageToWatt(v, r));
}

double RADAR_Tx::dbmToPeakVoltage(double dbm, double r)
{
	return wattToPeakVoltage(dbmToWatt(dbm), r);
}

double RADAR_Tx::deg2rad(double x)
{
	return x * M_PI / 180.0;
}

double RADAR_Tx::clamp(double x, double lo, double hi)
{
	if (x < lo) {
		return lo;
	}

	if (x > hi) {
		return hi;
	}

	return x;
}
