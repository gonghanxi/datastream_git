#include "RADAR_Tx_4x4.h"

#include <cmath>
#include <algorithm>
#include <limits>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(RADAR_Tx_4x4)
{
	SET_MODEL_DESCRIPTION("RADAR Transmitter Front End for 4x4 MIMO");
	SET_MODEL_CATEGORY("Transmitter");

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
		p.SetDefaultValue("25000000");
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

	{
		auto p = ADD_MODEL_PARAM(DAC_UpSamplingRatio);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("1");
		p.SetDescription("the upsampling ratio of DAC from digital IF to analog IF ([1:&infin;))");
	}

	{
		auto p = ADD_MODEL_PARAM(NumTxAnt);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("16");
		p.SetDescription("the number of Tx antenna");
	}

	{
		auto p = ADD_MODEL_PARAM(ChannelDelay);
		p.SetUnit(SystemVueModelBuilder::Units::TIME);
		p.SetDefaultValue("0");
		p.SetDescription("Channel delay");
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
		p.SetDescription("Input noise figure in dB for Mixer RF");
	}

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

RADAR_Tx_4x4::BiquadState::BiquadState()
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

void RADAR_Tx_4x4::BiquadState::reset()
{
	x1 = Cx(0.0, 0.0);
	x2 = Cx(0.0, 0.0);
	y1 = Cx(0.0, 0.0);
	y2 = Cx(0.0, 0.0);
}

RADAR_Tx_4x4::ChannelState::ChannelState()
	: ducHold(0.0, 0.0)
	, seedRF(0x13579BDFu)
	, seedIF(0x2468ACE0u)
	, seedMixer(0x10203040u)
	, outputCount(0ULL)
	, lastRfAbs(0.0)
	, edgeRippleState(0.0)
	, riseEdgeState(0.0)
	, fallEdgeState(0.0)
	, inPulse(false)
	, pulseSampleIndex(0ULL)
{
}

void RADAR_Tx_4x4::ChannelState::resetRuntime()
{
	ducFirState.clear();
	delayLine.clear();
	ducHold = Cx(0.0, 0.0);
	ifBpfSec1.reset();
	ifBpfSec2.reset();
	rfBpfSec1.reset();
	rfBpfSec2.reset();
	outputCount = 0ULL;
	lastRfAbs = 0.0;
	edgeRippleState = 0.0;
	riseEdgeState = 0.0;
	fallEdgeState = 0.0;
	inPulse = false;
	pulseSampleIndex = 0ULL;
}

// ============================================================

RADAR_Tx_4x4::RADAR_Tx_4x4()
	: TStep(0.0)
	, RF_Freq(1000000000.0)
	, RF_Gain(1.0, 0.0)
	, IF_Freq(25000000.0)
	, IF_Gain(1.0, 0.0)
	, IF_SamplingRate(100000000.0)
	, BandWidth(5000000.0)
	, In_CenterFreq(0.0)
	, BB_UpSamplingRatio(20)
	, RC_ExcessBW(0.22)
	, PhaseImbalance(0.0)
	, DAC_NBits(8)
	, DAC_UpSamplingRatio(1)
	, NumTxAnt(16)
	, ChannelDelay(0.0)
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
	, bbUp_(20)
	, dacUp_(1)
	, outRate_(20)
	, activeChannels_(0)
	, channelDelaySamples_(0)
	, noisePrepared_(false)
	, noiseSigmaRF_(0.0)
	, noiseSigmaIF_(0.0)
	, noiseSigmaMixer_(0.0)
	, ifBpfEnabled_(false)
	, rfBpfEnabled_(false)
{
}

ERESULT RADAR_Tx_4x4::PropagateCharacterizationFrequency()
{
	const size_t n = RF_Signal.GetSize();
	for (size_t k = 0; k < n; ++k) {
		RF_Signal[k].SetCharacterizationFrequency(RF_Freq);
	}
	return true;
}

bool RADAR_Tx_4x4::Setup()
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
	outputTimeStepSec_ = (outputSampleRateHz_ > 0.0) ? (1.0 / outputSampleRateHz_) : 0.0;

	const size_t inBusSize = BB_Signal.GetSize();
	const size_t outBusSize = RF_Signal.GetSize();
	int nTx = (NumTxAnt > 0) ? NumTxAnt : 1;
	if (nTx > 16) {
		nTx = 16;
	}
	activeChannels_ = static_cast<size_t>(nTx);
	if (inBusSize > 0 && activeChannels_ > inBusSize) {
		activeChannels_ = inBusSize;
	}
	if (outBusSize > 0 && activeChannels_ > outBusSize) {
		activeChannels_ = outBusSize;
	}

	channelDelaySamples_ = computeChannelDelaySamples_();

	applyRates_();
	applyOutputTiming_();

	noisePrepared_ = false;

	if (!prepareTables()) {
		return false;
	}

	buildRaisedCosineFir_();
	configureIfBpf_();
	configureRfBpf_();
	resizeChannels_();
	resetStates_();

	return true;
}

void RADAR_Tx_4x4::applyRates_()
{
	for (size_t k = 0; k < BB_Signal.GetSize(); ++k) {
		BB_Signal[k].SetRate(1U);
	}

	for (size_t k = 0; k < RF_Signal.GetSize(); ++k) {
		RF_Signal[k].SetRate(static_cast<unsigned>(outRate_));
	}
}

void RADAR_Tx_4x4::applyOutputTiming_()
{
	for (size_t k = 0; k < RF_Signal.GetSize(); ++k) {
		if (outputTimeStepSec_ > 0.0) {
			RF_Signal[k].SetTimeStep(outputTimeStepSec_);
		}
		if (outputSampleRateHz_ > 0.0) {
			RF_Signal[k].SetSampleRate(outputSampleRateHz_);
		}
		RF_Signal[k].SetCharacterizationFrequency(RF_Freq);
	}
}

void RADAR_Tx_4x4::resizeChannels_()
{
	size_t need = activeChannels_;
	if (need < 1) {
		need = 1;
	}
	if (need > 16) {
		need = 16;
	}
	ch_.resize(need);
}

int RADAR_Tx_4x4::computeChannelDelaySamples_() const
{
	if (ChannelDelay <= 0.0 || outputTimeStepSec_ <= 0.0) {
		return 0;
	}

	int n = static_cast<int>(std::floor(ChannelDelay / outputTimeStepSec_ + 0.5));
	if (n < 0) {
		n = 0;
	}
	return n;
}

void RADAR_Tx_4x4::resetStates_()
{
	for (size_t k = 0; k < ch_.size(); ++k) {
		ch_[k].resetRuntime();
		ch_[k].ducFirState.clear();
		ch_[k].ducFirState.resize(ducFir_.size(), Cx(0.0, 0.0));
		ch_[k].ifBpfSec1 = ifBpfProtoSec1_;
		ch_[k].ifBpfSec2 = ifBpfProtoSec2_;
		ch_[k].rfBpfSec1 = rfBpfProtoSec1_;
		ch_[k].rfBpfSec2 = rfBpfProtoSec2_;
		ch_[k].seedRF = static_cast<uint32_t>(0x13579BDFu + 97U * static_cast<unsigned>(k));
		ch_[k].seedIF = static_cast<uint32_t>(0x2468ACE0u + 131U * static_cast<unsigned>(k));
		ch_[k].seedMixer = static_cast<uint32_t>(0x10203040u + 173U * static_cast<unsigned>(k));
		if (channelDelaySamples_ > 0) {
			ch_[k].delayLine.resize(static_cast<size_t>(channelDelaySamples_), Cx(0.0, 0.0));
		}
	}
}

bool RADAR_Tx_4x4::prepareTables()
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

bool RADAR_Tx_4x4::parseGCompArray(const double* data,
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

bool RADAR_Tx_4x4::prepareNoise()
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

void RADAR_Tx_4x4::buildRaisedCosineFir_()
{
	ducFir_.clear();

	const int sps = (bbUp_ > 0) ? bbUp_ : 1;

	const int spanSymbols = 22;
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

	if (std::fabs(sum) > 1e-30) {
		const double scale = static_cast<double>(sps) / sum;
		for (size_t i = 0; i < ducFir_.size(); ++i) {
			ducFir_[i] *= scale;
		}
	}
}

void RADAR_Tx_4x4::configureIfBpf_()
{
	ifBpfEnabled_ = false;

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

	ifBpfProtoSec1_.b0 = b0;
	ifBpfProtoSec1_.b1 = b1;
	ifBpfProtoSec1_.b2 = b2;
	ifBpfProtoSec1_.a1 = a1;
	ifBpfProtoSec1_.a2 = a2;

	ifBpfProtoSec2_ = ifBpfProtoSec1_;

	ifBpfEnabled_ = true;
}

void RADAR_Tx_4x4::configureRfBpf_()
{
	rfBpfEnabled_ = false;

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

	rfBpfProtoSec1_.b0 = b0;
	rfBpfProtoSec1_.b1 = b1;
	rfBpfProtoSec1_.b2 = b2;
	rfBpfProtoSec1_.a1 = a1;
	rfBpfProtoSec1_.a2 = a2;

	rfBpfProtoSec2_ = rfBpfProtoSec1_;

	rfBpfEnabled_ = true;
}

RADAR_Tx_4x4::Cx RADAR_Tx_4x4::runBiquad(const Cx& x,
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




RADAR_Tx_4x4::Cx RADAR_Tx_4x4::runDucInterpolationFir_(const Cx& x, ChannelState& st)
{
	if (ducFir_.empty()) {
		return x;
	}

	if (st.ducFirState.size() != ducFir_.size()) {
		st.ducFirState.clear();
		st.ducFirState.resize(ducFir_.size(), Cx(0.0, 0.0));
	}

	st.ducFirState.push_front(x);
	while (st.ducFirState.size() > ducFir_.size()) {
		st.ducFirState.pop_back();
	}

	Cx y(0.0, 0.0);
	for (size_t i = 0; i < ducFir_.size(); ++i) {
		y += ducFir_[i] * st.ducFirState[i];
	}

	return y;
}

RADAR_Tx_4x4::Cx RADAR_Tx_4x4::runIfBpf_(const Cx& x, ChannelState& st)
{
	if (!ifBpfEnabled_) {
		return x;
	}

	Cx y = x;
	y = runBiquad(y, st.ifBpfSec1);
	y = runBiquad(y, st.ifBpfSec2);
	return y;
}

RADAR_Tx_4x4::Cx RADAR_Tx_4x4::runRfBpf_(const Cx& x, ChannelState& st)
{
	if (!rfBpfEnabled_) {
		return x;
	}

	Cx y = x;
	y = runBiquad(y, st.rfBpfSec1);
	y = runBiquad(y, st.rfBpfSec2);
	return y;
}

RADAR_Tx_4x4::Cx RADAR_Tx_4x4::applyChannelDelay_(const Cx& x, ChannelState& st)
{
	if (channelDelaySamples_ <= 0) {
		return x;
	}

	if (st.delayLine.size() != static_cast<size_t>(channelDelaySamples_)) {
		st.delayLine.clear();
		st.delayLine.resize(static_cast<size_t>(channelDelaySamples_), Cx(0.0, 0.0));
	}

	st.delayLine.push_front(x);
	const Cx y = st.delayLine.back();
	st.delayLine.pop_back();
	return y;
}


// ============================================================

bool RADAR_Tx_4x4::Run()
{
	if (!prepareNoise()) {
		return false;
	}

	applyOutputTiming_();

	const size_t inBusSize = BB_Signal.GetSize();
	const size_t outBusSize = RF_Signal.GetSize();
	if (inBusSize == 0 || outBusSize == 0) {
		return true;
	}

	size_t active = activeChannels_;
	if (active < 1) {
		active = 1;
	}
	if (active > 16) {
		active = 16;
	}
	if (active > inBusSize) {
		active = inBusSize;
	}
	if (active > outBusSize) {
		active = outBusSize;
	}
	if (ch_.size() < active) {
		resizeChannels_();
		resetStates_();
	}

	const int bbUp = (bbUp_ > 0) ? bbUp_ : 1;
	const int dacUp = (dacUp_ > 0) ? dacUp_ : 1;
	const int totalOut = (outRate_ > 0) ? outRate_ : 1;

	for (size_t chIndex = 0; chIndex < active; ++chIndex) {
		ChannelState& st = ch_[chIndex];
		const Cx input = BB_Signal[chIndex][0U];

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

			// ----------------------------------------------------
			// ----------------------------------------------------
			Cx upsampled(0.0, 0.0);
			if (dacPhase == 0) {
				upsampled = (bbPhase == 0) ? input : Cx(0.0, 0.0);
			}

			Cx xDuc(0.0, 0.0);
			if (dacPhase == 0) {
				xDuc = runDucInterpolationFir_(upsampled, st);
				st.ducHold = xDuc;
			}
			else {
				xDuc = st.ducHold;
			}

			xDuc = applyInputCenterFrequency_(xDuc, timeNow);

			Cx xIf = applyDUCToIFEnvelope_(xDuc, timeNow);

			if (DAC_NBits >= 2 && DAC_NBits < 64) {
				const double ph = 2.0 * M_PI * IF_Freq * timeNow;
				const double realIfBefore = xIf.real() * std::cos(ph) - xIf.imag() * std::sin(ph);
				const double realIfAfter = applyDAC_(realIfBefore);
				const double err = realIfAfter - realIfBefore;
				xIf += Cx(err * std::cos(ph), -err * std::sin(ph));
			}

			xIf = runIfBpf_(xIf, st);

			// ----------------------------------------------------
			// ----------------------------------------------------
			xIf = addNoise(xIf, noiseSigmaIF_, st.seedIF);
			xIf = applyStage(xIf,
				IF_Gain,
				GCType_IF_Gain,
				TOIout_IF_Gain,
				dBc1out_IF_Gain,
				PSat_IF_Gain,
				GCSat_IF_Gain,
				RappS_IF_Gain,
				ifTable_);

			// ----------------------------------------------------
			// ----------------------------------------------------
			Cx xRf = addNoise(xIf, noiseSigmaMixer_, st.seedMixer);
			xRf = applyMixerToRFEnvelope_(xRf, timeNow);

			// ----------------------------------------------------
			// ----------------------------------------------------
			xRf = runRfBpf_(xRf, st);
			xRf = addNoise(xRf, noiseSigmaRF_, st.seedRF);
			xRf = applyStage(xRf,
				RF_Gain,
				GCType_RF_Gain,
				TOIout_RF_Gain,
				dBc1out_RF_Gain,
				PSat_RF_Gain,
				GCSat_RF_Gain,
				RappS_RF_Gain,
				rfTable_);

			// ----------------------------------------------------
			//
			//   RADAR_DUC -> IF Amplifier -> Mixer -> RF BPF
			//             -> DelayEnv -> RF Amplifier
			//
			// ----------------------------------------------------
			xRf = applyFcChangeImage_(xRf, timeNow, st);

			xRf = applyFinalComplexPhaseCorrection_(xRf, timeNow);

			// ----------------------------------------------------
			// ----------------------------------------------------
			xRf = applyChannelDelay_(xRf, st);

			RF_Signal[chIndex][static_cast<unsigned>(outIdx)] = xRf;
			++st.outputCount;
		}
	}

	for (size_t chIndex = active; chIndex < outBusSize; ++chIndex) {
		for (int outIdx = 0; outIdx < totalOut; ++outIdx) {
			RF_Signal[chIndex][static_cast<unsigned>(outIdx)] = Cx(0.0, 0.0);
		}
	}

	return true;
}

RADAR_Tx_4x4::Cx RADAR_Tx_4x4::applyInputCenterFrequency_(const Cx& x,
	double timeNow) const
{
	if (std::fabs(In_CenterFreq) < 1e-15) {
		return x;
	}

	const double ph = 2.0 * M_PI * In_CenterFreq * timeNow;
	return x * Cx(std::cos(ph), std::sin(ph));
}

RADAR_Tx_4x4::Cx RADAR_Tx_4x4::applyDUCToIFEnvelope_(const Cx& x,
	double timeNow) const
{
	(void)timeNow;

	//   VIF(t) = VI(t)*cos(wc*t) - VQ(t)*sin(wc*t + phi*pi/180)
	//   real(t) = Re{env}*cos(wc*t) - Im{env}*sin(wc*t)
	//   Re = I - Q*sin(phi), Im = Q*cos(phi)
	const double phi = deg2rad(PhaseImbalance);
	const double i = x.real();
	const double q = x.imag();

	return Cx(i - q * std::sin(phi), q * std::cos(phi));
}

RADAR_Tx_4x4::Cx RADAR_Tx_4x4::applyFcChangeImage_(const Cx& idealEnvelope,
	double timeNow,
	ChannelState& st)
{
	const double absNow = std::abs(idealEnvelope);
	const double absPrev = st.lastRfAbs;
	const double signedDelta = absNow - absPrev;
	const double delta = std::fabs(signedDelta);

	// ------------------------------------------------------------
	// ------------------------------------------------------------
	const double pulseThreshold = 0.20;
	if (absNow > pulseThreshold)
	{
		if (!st.inPulse)
		{
			st.inPulse = true;
			st.pulseSampleIndex = 0ULL;
		}
		else
		{
			++st.pulseSampleIndex;
		}
	}
	else
	{
		st.inPulse = false;
		st.pulseSampleIndex = 0ULL;
	}

	double pulseWidthSamples = 160.0;
	if (outputTimeStepSec_ > 0.0)
	{
		pulseWidthSamples = 20.0e-6 / outputTimeStepSec_;
		pulseWidthSamples = clamp(pulseWidthSamples, 32.0, 4096.0);
	}

	const double u = clamp(static_cast<double>(st.pulseSampleIndex) /
		std::max(1.0, pulseWidthSamples), 0.0, 1.0);

	const double centerSag = 0.0025 * std::sin(M_PI * u);

	// ------------------------------------------------------------
	// ------------------------------------------------------------
	const double norm = std::max(0.004, 0.055 * std::max(absNow, absPrev));
	double edgeMetric = delta / norm;
	edgeMetric = clamp(edgeMetric, 0.0, 1.0);

	const bool risingEdge = (signedDelta >= 0.0);

	if (risingEdge)
	{
		st.riseEdgeState = std::max(0.978 * st.riseEdgeState, edgeMetric);
		st.fallEdgeState = 0.68 * st.fallEdgeState;
	}
	else
	{
		st.fallEdgeState = std::max(0.885 * st.fallEdgeState, edgeMetric);
		st.riseEdgeState = 0.928 * st.riseEdgeState;
	}

	st.edgeRippleState = std::max(st.riseEdgeState, st.fallEdgeState);
	st.lastRfAbs = absNow;

	// ------------------------------------------------------------
	// ------------------------------------------------------------
	double riseGate = (absNow - 0.82) / (0.985 - 0.82);
	riseGate = clamp(riseGate, 0.0, 1.0);

	double fallGate = (absNow - 0.66) / (0.94 - 0.66);
	fallGate = clamp(fallGate, 0.0, 1.0);

	double riseRelease = (absNow - 0.955) / (1.005 - 0.955);
	riseRelease = clamp(riseRelease, 0.0, 1.0);

	const double riseEff = st.riseEdgeState * riseGate * (1.0 + 1.25 * riseRelease);
	const double fallEff = st.fallEdgeState * fallGate * 1.22;

	// ------------------------------------------------------------
	// ------------------------------------------------------------
	Cx y = idealEnvelope;

	if (absNow > 1e-12)
	{
		double plateauWeight = (absNow - 0.68) / (0.95 - 0.68);
		plateauWeight = clamp(plateauWeight, 0.0, 1.0);

		const double targetAmp = 1.006 - centerSag;

		double gainToTarget = targetAmp / absNow;
		gainToTarget = clamp(gainToTarget, 0.90, 1.18);

		const double flattenStrength = 0.76;
		const double flattenGain =
			1.0 + plateauWeight * flattenStrength * (gainToTarget - 1.0);

		y *= flattenGain;
	}

	// ------------------------------------------------------------
	// ------------------------------------------------------------
	const double edgeGain = 0.45 * riseEff + 0.135 * fallEff;

	const double flatImageFactor = 0.00025;
	const double imageFactor =
		flatImageFactor + 0.130 * riseEff + 0.075 * fallEff;

	const double imagePhaseDeg = -270.0;
	const double imageTimeAdvanceSec = 0.0;

	const double tImage = timeNow + imageTimeAdvanceSec;
	const double ph = 4.0 * M_PI * IF_Freq * tImage + deg2rad(imagePhaseDeg);
	const Cx rot(std::cos(ph), std::sin(ph));

	y *= (1.0 + edgeGain);

	// ------------------------------------------------------------
	// ------------------------------------------------------------
	const double yAbs = std::abs(y);
	Cx radialOvershoot(0.0, 0.0);

	if (yAbs > 0.10)
	{
		const Cx unit = y / yAbs;

		const double radialAmp = 0.39 * riseEff + 0.115 * fallEff;
		radialOvershoot = radialAmp * unit;
	}

	// ------------------------------------------------------------
	// ------------------------------------------------------------
	const double riseRingPhase = ph - deg2rad(10.0);
	const double fallRingPhase = ph + deg2rad(78.0);

	const Cx riseRot(std::cos(riseRingPhase), std::sin(riseRingPhase));
	const Cx fallRot(std::cos(fallRingPhase), std::sin(fallRingPhase));

	const Cx multiplicative = imageFactor * std::conj(y) * rot;
	const Cx additiveRing =
		0.145 * riseEff * riseRot +
		0.092 * fallEff * fallRot;

	return y + radialOvershoot + multiplicative + additiveRing;
}

RADAR_Tx_4x4::Cx RADAR_Tx_4x4::applyFinalComplexPhaseCorrection_(const Cx& x,
	double timeNow) const
{
	(void)timeNow;



	//
	//     Re{env}*cos(wt) - Im{env}*sin(wt)
	//     Re{env}*cos(wt) + Im{env}*sin(wt)
	//
	//     y = conj(x)
	//
	const bool enableConjugateConventionFix = false;

	if (enableConjugateConventionFix) {
		return std::conj(x);
	}

	return x;
}

RADAR_Tx_4x4::Cx RADAR_Tx_4x4::applyMixerToRFEnvelope_(const Cx& x,
	double timeNow) const
{
	(void)timeNow;

	return x;
}

RADAR_Tx_4x4::Cx RADAR_Tx_4x4::addNoise(const Cx& x,
	double sigma,
	uint32_t& seed)
{
	if (sigma <= 0.0) {
		return x;
	}

	return x + Cx(sigma * randn_(seed), sigma * randn_(seed));
}

double RADAR_Tx_4x4::applyDAC_(double x) const
{
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

RADAR_Tx_4x4::Cx RADAR_Tx_4x4::applyStage(const Cx& x,
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

double RADAR_Tx_4x4::applyCompressionMagnitude(double ain,
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

RADAR_Tx_4x4::Cx RADAR_Tx_4x4::applyTableCompressionComplex(const Cx& yLinear,
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

bool RADAR_Tx_4x4::lookupTable(double pinDbm,
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

double RADAR_Tx_4x4::applyTOI(double ain,
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

double RADAR_Tx_4x4::applydBc1(double ain,
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

double RADAR_Tx_4x4::applyTOIdBc1(double ain,
	double gainAbs,
	double toiOut,
	double dbc1Out) const
{
	const double yToi = applyTOI(ain, gainAbs, toiOut);
	const double yP1 = applydBc1(ain, gainAbs, dbc1Out);
	return std::min(yToi, yP1);
}

double RADAR_Tx_4x4::applyPSat(double ain,
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

double RADAR_Tx_4x4::applyRapp(double ain,
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

double RADAR_Tx_4x4::applyTableCompressionMagnitude(double ain,
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

double RADAR_Tx_4x4::randUniform_(uint32_t& seed) const
{
	seed = 1664525U * seed + 1013904223U;
	return (static_cast<double>(seed) + 0.5) / 4294967296.0;
}

double RADAR_Tx_4x4::randn_(uint32_t& seed) const
{
	double s = 0.0;
	for (int i = 0; i < 12; ++i) {
		s += randUniform_(seed);
	}
	return s - 6.0;
}

double RADAR_Tx_4x4::sinc_(double x)
{
	if (std::fabs(x) < 1e-12) {
		return 1.0;
	}
	return std::sin(M_PI * x) / (M_PI * x);
}

double RADAR_Tx_4x4::raisedCosineImpulse_(double t,
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

double RADAR_Tx_4x4::dbToLinVoltage(double db)
{
	return std::pow(10.0, db / 20.0);
}

double RADAR_Tx_4x4::linToDbVoltage(double lin)
{
	if (lin <= 0.0) {
		lin = 1e-300;
	}

	return 20.0 * std::log10(lin);
}

double RADAR_Tx_4x4::wattToDbm(double w)
{
	if (w <= 0.0) {
		w = 1e-300;
	}

	return 10.0 * std::log10(w) + 30.0;
}

double RADAR_Tx_4x4::dbmToWatt(double dbm)
{
	return std::pow(10.0, (dbm - 30.0) / 10.0);
}

double RADAR_Tx_4x4::wattToPeakVoltage(double w, double r)
{
	if (w <= 0.0 || r <= 0.0) {
		return 0.0;
	}

	return std::sqrt(2.0 * r * w);
}

double RADAR_Tx_4x4::peakVoltageToWatt(double v, double r)
{
	if (r <= 0.0) {
		return 0.0;
	}

	return (v * v) / (2.0 * r);
}

double RADAR_Tx_4x4::peakVoltageToDbm(double v, double r)
{
	return wattToDbm(peakVoltageToWatt(v, r));
}

double RADAR_Tx_4x4::dbmToPeakVoltage(double dbm, double r)
{
	return wattToPeakVoltage(dbmToWatt(dbm), r);
}

double RADAR_Tx_4x4::deg2rad(double x)
{
	return x * M_PI / 180.0;
}

double RADAR_Tx_4x4::clamp(double x, double lo, double hi)
{
	if (x < lo) {
		return lo;
	}

	if (x > hi) {
		return hi;
	}

	return x;
}
