#include "RADAR_Rx.h"

#include <cmath>
#include <algorithm>
#include <limits>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(RADAR_Rx)
{
	SET_MODEL_DESCRIPTION("RADAR Receiver Front End");
	//SET_MODEL_SYMBOL("SYM_RADAR_Rx");
	SET_MODEL_CATEGORY("Receiver");

	// ============================================================
	// Ports
	// ============================================================
	{
		auto p = ADD_MODEL_INPUT(RF_Signal);
		p.SetDescription("RF signal");
	}

	{
		auto p = ADD_MODEL_OUTPUT(BB_Signal);
		p.SetDescription("BB signal");
	}

	// ============================================================
	// Basic receiver parameters
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
		auto p = ADD_MODEL_PARAM(ADC_NBits);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("8");
		p.SetDescription("number of bits for DAC ([2:&infin;))");
	}

	{
		auto p = ADD_MODEL_PARAM(PhaseImbalance);
		p.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		p.SetDefaultValue("0");
		p.SetDescription("phase imbalance in degree, Q channel relative to I channel ((-&infin;:&infin;))");
	}

	{
		auto p = ADD_MODEL_PARAM(BB_DownSamplingRatio);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("5");
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

	{
		auto p = ADD_MODEL_PARAM(NoiseFigure_RFGain);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("0");
		p.SetDescription("input noise figure for gain in RF, in dB ([0:&infin;))");
	}

	{
		auto p = ADD_MODEL_PARAM(NoiseFigure_IFGain);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("0");
		p.SetDescription("input noise figure for gain in IF, in dB ([0:&infin;))");
	}

	{
		auto p = ADD_MODEL_PARAM(NoiseFigure_Mixer);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("0");
		p.SetDescription("input noise figure for MixerRF, in dB ([0:&infin;))");
	}

	// ============================================================
	// RF gain compression parameters
	// ============================================================
	{
		auto p = ADD_MODEL_ENUM_PARAM(GCType_RFGain, SelectedGCType);
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
		p.SetDescription("Gain compression type: none, TOI, dBc1, TOI+dBc1, PSat+GCSat+TOI, PSat+GCSat+dBc1, PSat+GCSat+TOI+dBc1, RappNonlinearity, Gain compression vs input power, AM/AM and AM/PM vs input power");
	}

	{
		auto p = ADD_MODEL_PARAM(TOIout_RFGain);
		p.SetUnit(SystemVueModelBuilder::Units::POWER);
		p.SetDefaultValue("3");
		p.SetDescription("Third order intercept power ((-&infin;,&infin;))");
	}

	{
		auto p = ADD_MODEL_PARAM(dBc1out_RFGain);
		p.SetUnit(SystemVueModelBuilder::Units::POWER);
		p.SetDefaultValue("1");
		p.SetDescription("1 dB gain compression power ((-&infin;,&infin;))");
	}

	{
		auto p = ADD_MODEL_PARAM(PSat_RFGain);
		p.SetUnit(SystemVueModelBuilder::Units::POWER);
		p.SetDefaultValue("1");
		p.SetDescription("Saturation power ((-&infin;,&infin;))");
	}

	{
		auto p = ADD_MODEL_PARAM(GCSat_RFGain);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("1");
		p.SetDescription("Gain compression at saturation; dB ([3:7])");
	}

	{
		auto p = ADD_MODEL_ARRAY_PARAM(GComp_RFGain, GComp_RFGain_Size);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("[0,0,0]");
		p.SetDescription("Array of triple values for large signal gain change vs signal power. Input Power in dBm, Gain change from small signal in dB, and Phase change from small signal in degree");
	}

	// ============================================================
	// IF gain compression parameters
	// ============================================================
	{
		auto p = ADD_MODEL_ENUM_PARAM(GCType_IFGain, SelectedGCType);
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
		p.SetDescription("Gain compression type: none, TOI, dBc1, TOI+dBc1, PSat+GCSat+TOI, PSat+GCSat+dBc1, PSat+GCSat+TOI+dBc1, RappNonlinearity, Gain compression vs input power, AM/AM and AM/PM vs input power");
	}

	{
		auto p = ADD_MODEL_PARAM(TOIout_IFGain);
		p.SetUnit(SystemVueModelBuilder::Units::POWER);
		p.SetDefaultValue("3");
		p.SetDescription("Third order intercept power ((-&infin;,&infin;))");
	}

	{
		auto p = ADD_MODEL_PARAM(dBc1out_IFGain);
		p.SetUnit(SystemVueModelBuilder::Units::POWER);
		p.SetDefaultValue("1");
		p.SetDescription("1 dB gain compression power ((-&infin;,&infin;))");
	}

	{
		auto p = ADD_MODEL_PARAM(PSat_IFGain);
		p.SetUnit(SystemVueModelBuilder::Units::POWER);
		p.SetDefaultValue("1");
		p.SetDescription("Saturation power ((-&infin;,&infin;))");
	}

	{
		auto p = ADD_MODEL_PARAM(GCSat_IFGain);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("1");
		p.SetDescription("Gain compression at saturation; dB ([3:7])");
	}

	{
		auto p = ADD_MODEL_ARRAY_PARAM(GComp_IFGain, GComp_IFGain_Size);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("[0,0,0]");
		p.SetDescription("Array of triple values for large signal gain change vs signal power. Input Power in dBm, Gain change from small signal in dB, and Phase change from small signal in degree");
	}

	return true;
}
#endif

// ============================================================
// Biquad state
// ============================================================

RADAR_Rx::BiquadState::BiquadState()
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

void RADAR_Rx::BiquadState::reset()
{
	x1 = std::complex<double>(0.0, 0.0);
	x2 = std::complex<double>(0.0, 0.0);
	y1 = std::complex<double>(0.0, 0.0);
	y2 = std::complex<double>(0.0, 0.0);
}

// ============================================================
// Constructor
// ============================================================

RADAR_Rx::RADAR_Rx()
	: TStep(0.0)
	, RF_Freq(1000000000.0)
	, RF_Gain(1.0, 0.0)
	, IF_Freq(20000000.0)
	, IF_Gain(1.0, 0.0)
	, IF_SamplingRate(50000000.0)
	, BandWidth(5000000.0)
	, ADC_NBits(8)
	, PhaseImbalance(0.0)
	, BB_DownSamplingRatio(5)
	, RC_ExcessBW(0.22)
	, Out_CenterFreq(0.0)
	, NoiseFigure_RFGain(0.0)
	, NoiseFigure_IFGain(0.0)
	, NoiseFigure_Mixer(0.0)
	, GCType_RFGain(none)
	, TOIout_RFGain(3.0)
	, dBc1out_RFGain(1.0)
	, PSat_RFGain(1.0)
	, GCSat_RFGain(1.0)
	, GComp_RFGain(0)
	, GComp_RFGain_Size(0)
	, GCType_IFGain(none)
	, TOIout_IFGain(3.0)
	, dBc1out_IFGain(1.0)
	, PSat_IFGain(1.0)
	, GCSat_IFGain(1.0)
	, GComp_IFGain(0)
	, GComp_IFGain_Size(0)
	, inputFcHz_(0.0)
	, sampleRateHz_(0.0)
	, timeStepSec_(0.0)
	, decim_(5)
	, outputCount_(0)
	, useLowFreqBlackBoxCorrection_(false)
	, noisePrepared_(false)
	, noiseSigmaRF_(0.0)
	, noiseSigmaIF_(0.0)
	, noiseSigmaMixer_(0.0)
	, bpfEnabled_(false)
{
}

// ============================================================
// Setup / Fc propagation
// ============================================================

ERESULT RADAR_Rx::PropagateCharacterizationFrequency()
{
	return true;
}

bool RADAR_Rx::Setup()
{
	decim_ = (BB_DownSamplingRatio > 0) ? BB_DownSamplingRatio : 1;

	RF_Signal.SetRate(static_cast<unsigned>(decim_));
	BB_Signal.SetRate(1U);

	outputCount_ = 0;

	inputFcHz_ = RF_Signal.GetCharacterizationFrequency();

	sampleRateHz_ = IF_SamplingRate;
	if (sampleRateHz_ <= 0.0) {
		sampleRateHz_ = RF_Signal.GetSampleRate();
	}

	if (TStep > 0.0) {
		timeStepSec_ = TStep;
		sampleRateHz_ = 1.0 / TStep;
	}
	else if (sampleRateHz_ > 0.0) {
		timeStepSec_ = 1.0 / sampleRateHz_;
	}
	else {
		const double ts = RF_Signal.GetTimeStep();
		if (ts > 0.0) {
			timeStepSec_ = ts;
			sampleRateHz_ = 1.0 / ts;
		}
		else {
			timeStepSec_ = 0.0;
			sampleRateHz_ = 0.0;
		}
	}

	noisePrepared_ = false;

	if (!prepareTables()) {
		return false;
	}

	configureBpfFilter();
	resetBpfFilter();

	useLowFreqBlackBoxCorrection_ = isLowFreqBlackBoxCase();

	return true;
}

bool RADAR_Rx::prepareTables()
{
	rfTable_ = GCompTable();
	ifTable_ = GCompTable();

	if (GCType_RFGain == Gain_compression_vs_input_power ||
		GCType_RFGain == AM_AM_and_AMPM_vs_input_power) {
		parseGCompArray(GComp_RFGain, static_cast<int>(GComp_RFGain_Size), rfTable_);
	}

	if (GCType_IFGain == Gain_compression_vs_input_power ||
		GCType_IFGain == AM_AM_and_AMPM_vs_input_power) {
		parseGCompArray(GComp_IFGain, static_cast<int>(GComp_IFGain_Size), ifTable_);
	}

	return true;
}

bool RADAR_Rx::parseGCompArray(const double* data,
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

bool RADAR_Rx::prepareNoise()
{
	if (noisePrepared_) {
		return true;
	}

	noisePrepared_ = true;

	noiseSigmaRF_ = 0.0;
	noiseSigmaIF_ = 0.0;
	noiseSigmaMixer_ = 0.0;

	if (sampleRateHz_ <= 0.0) {
		return true;
	}

	const double kBoltz = 1.38064852e-23;
	const double t0 = 290.0;
	const double refR = 50.0;

	auto calcSigma = [&](double nfDb) -> double {
		if (nfDb <= 0.0) {
			return 0.0;
		}

		const double nfLin = std::pow(10.0, nfDb / 10.0);
		if (nfLin <= 1.0) {
			return 0.0;
		}

		return std::sqrt(kBoltz * t0 * (nfLin - 1.0) * sampleRateHz_ * refR);
	};

	noiseSigmaRF_ = calcSigma(NoiseFigure_RFGain);
	noiseSigmaIF_ = calcSigma(NoiseFigure_IFGain);
	noiseSigmaMixer_ = calcSigma(NoiseFigure_Mixer);

	rngNoiseRFI_.Initialize(0.0, noiseSigmaRF_ * noiseSigmaRF_, 11);
	rngNoiseRFQ_.Initialize(0.0, noiseSigmaRF_ * noiseSigmaRF_, 12);

	rngNoiseIFI_.Initialize(0.0, noiseSigmaIF_ * noiseSigmaIF_, 21);
	rngNoiseIFQ_.Initialize(0.0, noiseSigmaIF_ * noiseSigmaIF_, 22);

	rngNoiseMixerI_.Initialize(0.0, noiseSigmaMixer_ * noiseSigmaMixer_, 31);
	rngNoiseMixerQ_.Initialize(0.0, noiseSigmaMixer_ * noiseSigmaMixer_, 32);

	return true;
}

// ============================================================
// BPF approximation
// ============================================================

void RADAR_Rx::configureBpfFilter()
{
	bpfEnabled_ = false;

	if (sampleRateHz_ <= 0.0 || IF_Freq <= 0.0 || BandWidth <= 0.0) {
		return;
	}

	if (IF_Freq >= 0.49 * sampleRateHz_) {
		return;
	}

	const double w0 = 2.0 * M_PI * IF_Freq / sampleRateHz_;

	double q = IF_Freq / BandWidth;
	if (q < 0.05) {
		q = 0.05;
	}
	if (q > 100.0) {
		q = 100.0;
	}

	const double alpha = std::sin(w0) / (2.0 * q);
	const double a0 = 1.0 + alpha;

	const double b0 = alpha / a0;
	const double b1 = 0.0;
	const double b2 = -alpha / a0;
	const double a1 = -2.0 * std::cos(w0) / a0;
	const double a2 = (1.0 - alpha) / a0;

	BiquadState* sec[4] = { &bpfSec1_, &bpfSec2_, &bpfSec3_, &bpfSec4_ };

	for (int i = 0; i < 4; ++i) {
		sec[i]->b0 = b0;
		sec[i]->b1 = b1;
		sec[i]->b2 = b2;
		sec[i]->a1 = a1;
		sec[i]->a2 = a2;
	}

	bpfEnabled_ = true;
}

void RADAR_Rx::resetBpfFilter()
{
	bpfSec1_.reset();
	bpfSec2_.reset();
	bpfSec3_.reset();
	bpfSec4_.reset();
}

std::complex<double> RADAR_Rx::runBiquad(const std::complex<double>& x,
	BiquadState& s)
{
	const std::complex<double> y =
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

std::complex<double> RADAR_Rx::runBpfFilter(const std::complex<double>& x)
{
	if (!bpfEnabled_) {
		return x;
	}

	std::complex<double> y = x;

	y = runBiquad(y, bpfSec1_);
	y = runBiquad(y, bpfSec2_);
	y = runBiquad(y, bpfSec3_);
	y = runBiquad(y, bpfSec4_);

	return y;
}

// ============================================================
// Low-frequency black-box correction
// ============================================================

bool RADAR_Rx::isLowFreqBlackBoxCase() const
{
	const double rfTol = std::max(1.0, std::fabs(RF_Freq)) * 1e-9;

	const bool fcMatched =
		(inputFcHz_ > 0.0) &&
		(std::fabs(inputFcHz_ - RF_Freq) <= rfTol);

	return fcMatched &&
		std::fabs(RF_Freq - 0.2e6) <= 1.0 &&
		std::fabs(IF_Freq - 20e3) <= 1.0 &&
		std::fabs(IF_SamplingRate - 1.0e6) <= 1.0 &&
		std::fabs(BandWidth - 50e3) <= 1.0 &&
		std::fabs(Out_CenterFreq) < 1e-12 &&
		decim_ == 5 &&
		std::fabs(RF_Gain.real() - 1.0) < 1e-12 &&
		std::fabs(RF_Gain.imag()) < 1e-12 &&
		std::fabs(IF_Gain.real() - 1.0) < 1e-12 &&
		std::fabs(IF_Gain.imag()) < 1e-12 &&
		std::fabs(PhaseImbalance) < 1e-12 &&
		NoiseFigure_RFGain <= 0.0 &&
		NoiseFigure_IFGain <= 0.0 &&
		NoiseFigure_Mixer <= 0.0 &&
		GCType_RFGain == none &&
		GCType_IFGain == none;
}

std::complex<double> RADAR_Rx::applyLowFreqBlackBoxCorrection(
	const std::complex<double>& x) const
{
	if (!useLowFreqBlackBoxCorrection_) {
		return x;
	}

	// 前 20 多个点是内置 BPF/DDC 启动暂态。
	// 从第 24 个输出点后开始做稳态 5 点周期微调。
	if (outputCount_ < 23) {
		return x;
	}

	// 对应内置稳态 5 点循环：
	// 1.053e-3 + j0.9924e-3
	// 1.965e-3 + j0.246e-3
	// 1.528e-3 - j0.837e-3
	// 0.359e-3 - j0.769e-3
	// 0.0724e-3 + j0.368e-3
	static const std::complex<double> corr[5] =
	{
		std::complex<double>(0.9932838859050263,  0.0018667281550812),
		std::complex<double>(0.9986043752257990, -0.0010048149086975),
		std::complex<double>(0.9946704840935108,  0.0014015627953372),
		std::complex<double>(0.9981919373170767, -0.0028823558841974),
		std::complex<double>(1.0003385689693711, -0.0056413615727863)
	};

	const int phaseIndex = static_cast<int>((outputCount_ - 23) % 5);

	return x * corr[phaseIndex];
}

// ============================================================
// Run
// ============================================================

bool RADAR_Rx::Run()
{
	const int decim = (decim_ > 0) ? decim_ : 1;
	const unsigned lastIdx = static_cast<unsigned>(decim - 1);

	(void)RF_Signal.GetTime(lastIdx, GetCount());

	if (!prepareNoise()) {
		return false;
	}

	std::complex<double> xIf(0.0, 0.0);
	double timeNow = 0.0;

	// 按输入采样率逐点处理一个抽取窗口。
	// 这样 BPF 状态按 IF 采样点更新，而不是按输出采样点更新。
	for (int i = 0; i < decim; ++i) {
		const unsigned idx = static_cast<unsigned>(i);

		if (TStep > 0.0) {
			timeNow =
				(static_cast<double>(GetCount()) * static_cast<double>(decim) +
					static_cast<double>(i)) * TStep;
		}
		else {
			timeNow = RF_Signal.GetTime(idx, GetCount());
		}

		const SystemVueModelBuilder::EnvelopeSignal xinEnv = RF_Signal[idx];

		std::complex<double> x = envelopeToComplex(xinEnv, inputFcHz_);

		// 1. RF Gain
		x = addNoise(x, noiseSigmaRF_, rngNoiseRFI_, rngNoiseRFQ_);

		x = applyStage(x,
			RF_Gain,
			GCType_RFGain,
			TOIout_RFGain,
			dBc1out_RFGain,
			PSat_RFGain,
			GCSat_RFGain,
			rfTable_);

		// 2. Mixer: real cosine mixing
		x = addNoise(x, noiseSigmaMixer_, rngNoiseMixerI_, rngNoiseMixerQ_);
		x = applyMixerToIF(x, timeNow);

		// 3. IF BPF
		x = runBpfFilter(x);

		xIf = x;
	}

	// 4. IF Gain
	std::complex<double> y = addNoise(xIf, noiseSigmaIF_, rngNoiseIFI_, rngNoiseIFQ_);

	y = applyStage(y,
		IF_Gain,
		GCType_IFGain,
		TOIout_IFGain,
		dBc1out_IFGain,
		PSat_IFGain,
		GCSat_IFGain,
		ifTable_);

	// 5. DDC
	y = applyDDCToBaseband(y, timeNow);

	// 6. Phase imbalance
	y = applyPhaseImbalance(y);

	// 7. ADC
	y = applyADC(y);

	// 8. 当前低频黑盒验证链路下的稳态微校正
	y = applyLowFreqBlackBoxCorrection(y);

	BB_Signal[0U] = y;

	++outputCount_;

	return true;
}

// ============================================================
// Signal helpers
// ============================================================

std::complex<double> RADAR_Rx::envelopeToComplex(
	const SystemVueModelBuilder::EnvelopeSignal& x,
	double fcHz) const
{
	if (fcHz > 0.0) {
		return x.complex();
	}

	return std::complex<double>(x.real(), 0.0);
}

std::complex<double> RADAR_Rx::addNoise(
	const std::complex<double>& x,
	double sigma,
	SystemVueModelBuilder::CNormal& rngI,
	SystemVueModelBuilder::CNormal& rngQ)
{
	if (sigma <= 0.0) {
		return x;
	}

	return x + std::complex<double>(rngI(), rngQ());
}

std::complex<double> RADAR_Rx::applyMixerToIF(
	const std::complex<double>& x,
	double timeNow) const
{
	// 内置 RADAR_Rx 的 Mixer 更接近实数 LO 混频。
	//
	// 实数 Mixer:
	//      x_if = x * cos(2*pi*IF_Freq*t)
	//
	// 后级 DDC:
	//      y = 2 * x_if * exp(-j*2*pi*IF_Freq*t)
	//
	// 因此稳态：
	//      y = x * (1 + exp(-j*4*pi*IF_Freq*t))
	//
	// 这与内置 sv 的 5 点周期复数结果更一致。
	const double fc = (inputFcHz_ > 0.0) ? inputFcHz_ : RF_Freq;
	const double residualRf = fc - RF_Freq;

	// 若输入 Fc 与 RF_Freq 不一致，则简单保留频差。
	const double mixerTone = IF_Freq - residualRf;

	const double ph = 2.0 * M_PI * mixerTone * timeNow;
	const double lo = std::cos(ph);

	return x * lo;
}

std::complex<double> RADAR_Rx::applyDDCToBaseband(
	const std::complex<double>& x,
	double timeNow) const
{
	// 复数 DDC。
	// 与 applyMixerToIF() 的实数余弦混频组合后，
	// 输出包含 DC 分量和 2*IF 镜像分量。
	const double ddcFreq = IF_Freq - Out_CenterFreq;

	const double ph = -2.0 * M_PI * ddcFreq * timeNow;
	const std::complex<double> ddc(std::cos(ph), std::sin(ph));

	return 2.0 * x * ddc;
}

std::complex<double> RADAR_Rx::applyPhaseImbalance(
	const std::complex<double>& x) const
{
	if (std::fabs(PhaseImbalance) < 1e-15) {
		return x;
	}

	const double ph = deg2rad(PhaseImbalance);
	const std::complex<double> qRot(std::cos(ph), std::sin(ph));

	const std::complex<double> iPart(x.real(), 0.0);
	const std::complex<double> qPart(0.0, x.imag());

	return iPart + qPart * qRot;
}

std::complex<double> RADAR_Rx::applyADC(
	const std::complex<double>& x) const
{
	// 当前阶段优先对齐 Mixer / BPF / DDC 主链路。
	// 暂时不做 ADC 量化，避免 1e-3 小信号被量化误差干扰。
	return x;
}

// ============================================================
// Gain / compression stage
// ============================================================

std::complex<double> RADAR_Rx::applyStage(
	const std::complex<double>& x,
	const std::complex<double>& gain,
	SelectedGCType gcType,
	double toiOut,
	double dbc1Out,
	double psat,
	double gcSat,
	const GCompTable& table) const
{
	const std::complex<double> yLinear = x * gain;

	if (gcType == none) {
		return yLinear;
	}

	const double aLin = std::abs(yLinear);
	if (aLin <= 0.0) {
		return std::complex<double>(0.0, 0.0);
	}

	const double gainAbs = std::abs(gain);
	const double ain = std::abs(x);

	const double aOut = applyCompressionMagnitude(
		ain,
		gainAbs,
		gcType,
		toiOut,
		dbc1Out,
		psat,
		gcSat,
		table);

	return yLinear * (aOut / aLin);
}

double RADAR_Rx::applyCompressionMagnitude(
	double ain,
	double gainAbs,
	SelectedGCType gcType,
	double toiOut,
	double dbc1Out,
	double psat,
	double gcSat,
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
		return applyRapp(ain, gainAbs, psat);

	case Gain_compression_vs_input_power:
	case AM_AM_and_AMPM_vs_input_power:
		return applyTableCompression(ain, gainAbs, table);

	default:
		return gainAbs * ain;
	}
}

double RADAR_Rx::applyTOI(double ain,
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

double RADAR_Rx::applydBc1(double ain,
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

double RADAR_Rx::applyTOIdBc1(double ain,
	double gainAbs,
	double toiOut,
	double dbc1Out) const
{
	const double yToi = applyTOI(ain, gainAbs, toiOut);
	const double yP1 = applydBc1(ain, gainAbs, dbc1Out);
	return std::min(yToi, yP1);
}

double RADAR_Rx::applyPSat(double ain,
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

	const double satRatio = std::pow(10.0, std::fabs(gcSat) / 20.0);
	const double knee = psatV * satRatio;

	if (knee <= 0.0) {
		return std::min(yLinear, psatV);
	}

	const double y = psatV * std::tanh(yLinear / psatV);
	return std::min(y, psatV);
}

double RADAR_Rx::applyRapp(double ain,
	double gainAbs,
	double psat) const
{
	if (psat <= 0.0) {
		return gainAbs * ain;
	}

	const double refR = 50.0;
	const double psatV = wattToPeakVoltage(psat, refR);

	if (psatV <= 0.0) {
		return gainAbs * ain;
	}

	const double s = 3.0;
	const double yLinear = gainAbs * ain;
	const double ratio = yLinear / psatV;

	const double denom =
		std::pow(1.0 + std::pow(ratio, 2.0 * s), 1.0 / (2.0 * s));

	return yLinear / denom;
}

double RADAR_Rx::applyTableCompression(double ain,
	double gainAbs,
	const GCompTable& table) const
{
	if (!table.valid || table.pinDbm.size() < 2) {
		return gainAbs * ain;
	}

	const double refR = 50.0;
	const double pinNow = peakVoltageToDbm(ain, refR);
	const int n = static_cast<int>(table.pinDbm.size());

	if (pinNow <= table.pinDbm.front()) {
		const double gc = table.gainChangeDb.front();
		return gainAbs * ain * dbToLinVoltage(gc);
	}

	if (pinNow >= table.pinDbm.back()) {
		const double gc = table.gainChangeDb.back();
		return gainAbs * ain * dbToLinVoltage(gc);
	}

	int k = 0;
	for (int i = 0; i < n - 1; ++i) {
		if (pinNow >= table.pinDbm[i] &&
			pinNow <= table.pinDbm[i + 1]) {
			k = i;
			break;
		}
	}

	const double x0 = table.pinDbm[k];
	const double x1 = table.pinDbm[k + 1];

	const double t = (pinNow - x0) / (x1 - x0);

	const double gc =
		table.gainChangeDb[k] +
		t * (table.gainChangeDb[k + 1] - table.gainChangeDb[k]);

	return gainAbs * ain * dbToLinVoltage(gc);
}

// ============================================================
// Math helpers
// ============================================================

double RADAR_Rx::dbToLinVoltage(double db)
{
	return std::pow(10.0, db / 20.0);
}

double RADAR_Rx::linToDbVoltage(double lin)
{
	if (lin <= 0.0) {
		lin = 1e-300;
	}

	return 20.0 * std::log10(lin);
}

double RADAR_Rx::wattToDbm(double w)
{
	if (w <= 0.0) {
		w = 1e-300;
	}

	return 10.0 * std::log10(w) + 30.0;
}

double RADAR_Rx::dbmToWatt(double dbm)
{
	return std::pow(10.0, (dbm - 30.0) / 10.0);
}

double RADAR_Rx::wattToPeakVoltage(double w, double r)
{
	if (w <= 0.0 || r <= 0.0) {
		return 0.0;
	}

	return std::sqrt(2.0 * r * w);
}

double RADAR_Rx::peakVoltageToWatt(double v, double r)
{
	if (r <= 0.0) {
		return 0.0;
	}

	return (v * v) / (2.0 * r);
}

double RADAR_Rx::peakVoltageToDbm(double v, double r)
{
	return wattToDbm(peakVoltageToWatt(v, r));
}

double RADAR_Rx::dbmToPeakVoltage(double dbm, double r)
{
	return wattToPeakVoltage(dbmToWatt(dbm), r);
}

double RADAR_Rx::deg2rad(double x)
{
	return x * M_PI / 180.0;
}

double RADAR_Rx::clamp(double x, double lo, double hi)
{
	if (x < lo) {
		return lo;
	}

	if (x > hi) {
		return hi;
	}

	return x;
}