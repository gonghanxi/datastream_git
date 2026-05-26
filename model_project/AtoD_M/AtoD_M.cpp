#include "AtoD_M.h"

#include <algorithm>
#include <ctime>
#include <cstdlib>

const double AtoD_M::kPi = 3.1415926535897932384626433832795;
const double AtoD_M::kTiny = 1e-30;

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(AtoD_M)
{
	SET_MODEL_DESCRIPTION("Analog to Digital Converter");
	SET_MODEL_SYMBOL("SYM_AtoD");
	SET_MODEL_CATEGORY("Algorithm Design");

	// --------- 端口 ---------
	{
		SystemVueModelBuilder::DFPort p = ADD_MODEL_INPUT(A_in);
		p.SetDescription("input matrix analog signal");
	}
	{
		SystemVueModelBuilder::DFPort p = ADD_MODEL_OUTPUT(A_out);
		p.SetDescription("output matrix sampled analog baseband signal");
	}
	{
		SystemVueModelBuilder::DFPort p = ADD_MODEL_OUTPUT(D_I);
		p.SetDescription("output NBit word as integer for I channel");
	}
	{
		SystemVueModelBuilder::DFPort p = ADD_MODEL_OUTPUT(D_Q);
		p.SetDescription("output NBit word as integer for Q channel");
	}

	// --------- 基本参数 ---------
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(NBits);
		p.SetDefaultValue("8");
		p.SetDescription("Number of bits");
	}
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(VRef);
		p.SetDefaultValue("1.0");
		p.SetUnit(SystemVueModelBuilder::Units::VOLTAGE);
		p.SetDescription("Reference voltage, -VRef<=input<=VRef");
	}
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_ENUM_PARAM(OutputDigitalFormat, OutputDigitalFormatEnum);
		p.AddEnumeration("Offset binary", AtoD_M::Offset_binary);
		p.AddEnumeration("Twos-complement", AtoD_M::Twos_complement);
		p.SetDefaultValue("Offset binary");
		p.SetDescription("Output digital format: Offset binary, Twos-complement");
	}
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_ENUM_PARAM(DistortionModel, DistortionModelEnum);
		p.AddEnumeration("None", AtoD_M::Distortion_None);
		p.AddEnumeration("Jitter/INL/DNL", AtoD_M::Jitter_INL_DNL);
		p.AddEnumeration("ENOB value", AtoD_M::ENOB_value);
		p.AddEnumeration("SNR and Harmonics", AtoD_M::SNR_and_Harmonics);
		p.AddEnumeration("SINAD and SFDR", AtoD_M::SINAD_and_SFDR);
		p.SetDefaultValue("Jitter/INL/DNL");
		p.SetDescription("Distortion model: None, Jitter/INL/DNL, ENOB value, SNR and Harmonics, SINAD and SFDR");
	}

	// --------- Jitter / INL / DNL 参数 ---------
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_ENUM_PARAM(EnableJitter, EnableJitterEnum);
		p.AddEnumeration("No", AtoD_M::Jitter_No);
		p.AddEnumeration("Time Domain", AtoD_M::Time_Domain);
		p.AddEnumeration("Frequency Domain", AtoD_M::Frequency_Domain);
		p.SetDefaultValue("No");
		p.SetDescription("Enable jitter: No, Time Domain, Frequency Domain");
		p.SetHideCondition("DistortionModel ~= 1");
	}
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(RJrms);
		p.SetDefaultValue("0.0");
		p.SetUnit(SystemVueModelBuilder::Units::TIME);
		p.SetDescription("Random jitter standard deviation");
		p.SetHideCondition("DistortionModel ~= 1 || EnableJitter ~= 1");
	}
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_ARRAY_PARAM(PhaseNoiseData, PhaseNoiseDataSize);
		p.SetDefaultValue("");
		p.SetDescription("Phase noise specification - pairs of offset freq (Hz) and SSB phase noise level (dBc/Hz)");
		p.SetHideCondition("DistortionModel ~= 1 || EnableJitter ~= 2");
	}
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_ENUM_PARAM(PN_Type, PN_TypeEnum);
		p.AddEnumeration("Random PN", AtoD_M::Random_PN);
		p.AddEnumeration("Fixed freq offset", AtoD_M::Fixed_freq_offset);
		p.AddEnumeration("Fixed freq offset and amplitude", AtoD_M::Fixed_freq_offset_and_amplitude);
		p.SetDefaultValue("Random PN");
		p.SetDescription("Phase noise model type with random or fixed offset freq spacing and amplitude: Random PN, Fixed freq offset, Fixed freq offset and amplitude");
		p.SetHideCondition("DistortionModel ~= 1 || EnableJitter ~= 2");
	}
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(INL);
		p.SetDefaultValue("0.0");
		p.SetDescription("Integral nonlinearity relative to least significant bit (LSB)");
		p.SetHideCondition("DistortionModel ~= 1");
	}
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(DNL);
		p.SetDefaultValue("0.0");
		p.SetDescription("Differential nonlinearity relative to least significant bit (LSB)");
		p.SetHideCondition("DistortionModel ~= 1");
	}

	// --------- ENOB 参数 ---------
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(ENOB);
		p.SetDefaultValue("7");
		p.SetDescription("Equivalent number of bits (based on INL and DNL)");
		p.SetHideCondition("DistortionModel ~= 2");
	}

	// --------- SNR and Harmonics 参数 ---------
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(SNR_dB);
		p.SetDefaultValue("60.0");
		p.SetDescription("SNR output in dB for analog input");
		p.SetHideCondition("DistortionModel ~= 3");
	}
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(H2_dBc);
		p.SetDefaultValue("-400.0");
		p.SetDescription("2nd harmonic output level in dBc relative to fundamental output");
		p.SetHideCondition("DistortionModel ~= 3");
	}
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(H3_dBc);
		p.SetDefaultValue("-400.0");
		p.SetDescription("3rd harmonic output level in dBc relative to fundamental output");
		p.SetHideCondition("DistortionModel ~= 3");
	}
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(H4_dBc);
		p.SetDefaultValue("-400.0");
		p.SetDescription("4th harmonic output level in dBc relative to fundamental output");
		p.SetHideCondition("DistortionModel ~= 3");
	}
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(H5_dBc);
		p.SetDefaultValue("-400.0");
		p.SetDescription("5th harmonic output level in dBc relative to fundamental output");
		p.SetHideCondition("DistortionModel ~= 3");
	}

	// --------- SINAD and SFDR 参数 ---------
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(SINAD_dB);
		p.SetDefaultValue("60.0");
		p.SetDescription("Output signal to (noise plus harmonic distortion) ratio in dB");
		p.SetHideCondition("DistortionModel ~= 4");
	}
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(SFDR_dBc);
		p.SetDefaultValue("70.0");
		p.SetDescription("Output spurious free dynamic range in dBc relative to fundamental output level");
		p.SetHideCondition("DistortionModel ~= 4");
	}
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_ENUM_PARAM(FFT_Size, FFT_SizeEnum);
		p.AddEnumeration("2^12", AtoD_M::FFT_2_12);
		p.AddEnumeration("2^13", AtoD_M::FFT_2_13);
		p.AddEnumeration("2^14", AtoD_M::FFT_2_14);
		p.AddEnumeration("2^15", AtoD_M::FFT_2_15);
		p.AddEnumeration("2^16", AtoD_M::FFT_2_16);
		p.SetDefaultValue("2^14");
		p.SetDescription("FFT size as power of 2: 2^12, 2^13, 2^14, 2^15, 2^16");
		p.SetHideCondition("DistortionModel ~= 4");
	}

	// --------- SNR_Model 及其相关参数 ---------
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_ENUM_PARAM(SNR_Model, SNR_ModelEnum);
		p.AddEnumeration("Quantization_and_Jitter", AtoD_M::Quantization_and_Jitter);
		p.AddEnumeration("Quantization_and_INL_DNL", AtoD_M::Quantization_and_INL_DNL);
		p.AddEnumeration("Quantization_and_Jitter_or_INL_DNL", AtoD_M::Quantization_and_Jitter_or_INL_DNL);
		p.AddEnumeration("Quantization_Jitter_and_Thermal_Noise", AtoD_M::Quantization_Jitter_and_Thermal_Noise);
		p.SetDefaultValue("Quantization_and_Jitter");
		p.SetDescription("SNR model: Quantization_and_Jitter, Quantization_and_INL_DNL, Quantization_and_Jitter_or_INL_DNL, Quantization_Jitter_and_Thermal_Noise");
		p.SetHideCondition("DistortionModel == 0 || DistortionModel == 1");
	}
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(ThermalNoise_SNR_dBFS);
		p.SetDefaultValue("63");
		p.SetDescription("Thermal noise level in dBFS");
		p.SetHideCondition("(DistortionModel == 0 || DistortionModel == 1) || SNR_Model ~= 3");
	}
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(CenterFreq);
		p.SetDefaultValue("100.0e6");
		p.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		p.SetDescription("Spectral center frequency for analog input");
		p.SetHideCondition("(DistortionModel == 0 || DistortionModel == 1) || SNR_Model == 1");
	}
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(Level_dBFS);
		p.SetDefaultValue("0.0");
		p.SetDescription("Signal level in dBFS for analog input");
		p.SetHideCondition("DistortionModel == 0 || DistortionModel == 1");
	}

	// --------- 转换方式参数 ---------
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_ENUM_PARAM(ConversionType, ConversionTypeEnum);
		p.AddEnumeration("Clocked", AtoD_M::Clocked);
		p.AddEnumeration("Downsampled", AtoD_M::Downsampled);
		p.SetDefaultValue("Clocked");
		p.SetDescription("Type of input conversion: Clocked, Downsampled");
	}
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(Clock);
		p.SetDefaultValue("0.2e6");
		p.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		p.SetDescription("Internal cosine clock frequency");
		p.SetHideCondition("ConversionType ~= 0");
	}
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(Phase);
		p.SetDefaultValue("0.0");
		p.SetDescription("Internal clock phase");
		p.SetHideCondition("ConversionType ~= 0");
	}
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(DownsampleFactor);
		p.SetDefaultValue("1");
		p.SetDescription("Downsampling ratio");
		p.SetHideCondition("ConversionType ~= 1");
	}
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(DownsamplePhase);
		p.SetDefaultValue("0");
		p.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		p.SetDescription("Downsampling phase");
		p.SetHideCondition("ConversionType ~= 1");
	}
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_ENUM_PARAM(AntiAliasingFilter, AntiAliasingFilterEnum);
		p.AddEnumeration("OFF", AtoD_M::AA_OFF);
		p.AddEnumeration("ON", AtoD_M::AA_ON);
		p.SetDefaultValue("OFF");
		p.SetDescription("Turn off/on anti-aliasing filter before downsampling: OFF, ON");
		p.SetHideCondition("ConversionType ~= 1");
	}
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(ExcessBW);
		p.SetDefaultValue("0.5");
		p.SetDescription("Excess bandwidth of raised cosine anti-aliasing filter");
		p.SetHideCondition("ConversionType ~= 1 || AntiAliasingFilter ~= 1");
	}

	return true;
}
#endif


AtoD_M::AtoD_M(): NBits(8),
	 VRef(1.0),
	 OutputDigitalFormat(Offset_binary),
	 DistortionModel(Jitter_INL_DNL),
	 EnableJitter(Jitter_No),
	 RJrms(0.0),
	 PhaseNoiseData(nullptr),
	 PhaseNoiseDataSize(0),
	 PN_Type(Random_PN),
	 INL(0.0),
	 DNL(0.0),
	 ENOB(7.0),
	 SNR_dB(60.0),
	 H2_dBc(-400.0),
	 H3_dBc(-400.0),
	 H4_dBc(-400.0),
	 H5_dBc(-400.0),
	 SINAD_dB(60.0),
	 SFDR_dBc(70.0),
	 FFT_Size(FFT_2_14),
	 SNR_Model(Quantization_and_Jitter),
	 ThermalNoise_SNR_dBFS(63.0),
	 CenterFreq(100.0e6),
	 Level_dBFS(0.0),
	 ConversionType(Clocked),
	 Clock(0.2e6),
	 Phase(0.0),
	 DownsampleFactor(1),
	 DownsamplePhase(0),
	 AntiAliasingFilter(AA_OFF),
	 ExcessBW(0.5),
	 nbits_(8),
	 codeCount_(256),
	 midCode_(128),
	 vref_(1.0),
	 lsb_(2.0 / 256.0),
	 sampleIndex_(0ULL),
	 rngState_(0x12345678U)
{
}


ERESULT AtoD_M::PropagateCharacterizationFrequency()
{
	A_out.SetCharacterizationFrequency(A_in.GetCharacterizationFrequency());
	return true;
}


bool AtoD_M::Setup()
{
	clamp_params_();

	if (ConversionType == Downsampled)
		try_set_rate_(A_in, DownsampleFactor, 0);
	else
		try_set_rate_(A_in, 1, 0);

	try_set_rate_(A_out, 1, 0);
	try_set_rate_(D_I, 1, 0);
	try_set_rate_(D_Q, 1, 0);

	build_transfer_table_();

	sampleIndex_ = 0ULL;
	reset_states_();

	// 随机模型使用非固定初值，使 Time Domain Jitter / ENOB / SNR 噪声每次运行可变化。
	rngState_ = static_cast<unsigned int>(std::time(nullptr)) ^ 0x9E3779B9U;

	A_out.SetCharacterizationFrequency(A_in.GetCharacterizationFrequency());

	return true;
}


bool AtoD_M::Run()
{
	double t = A_out.GetTime(0, GetCount());

    const SystemVueModelBuilder::EnvelopeMatrix& in0 = A_Input[0];
	const std::size_t rows = in0.NumRows();
	const std::size_t cols = in0.NumColumns();
	const std::size_t elemCount = in0.NumElements();

	ensure_state_count_(elemCount);

	std::vector<std::complex<double> > sampled(elemCount, std::complex<double>(0.0, 0.0));

	if (ConversionType == Downsampled)
	{
		sampled = get_downsampled_matrix_(elemCount);
	}
	else
	{
		for (std::size_t i = 0; i < elemCount; ++i)
		{
			const std::complex<double> xin = in0(i).complex();
			sampled[i] = get_clocked_input_(xin, t, clockStates_[i]);
		}
	}

	SystemVueModelBuilder::EnvelopeMatrix outA;
	SystemVueModelBuilder::IntMatrix outI;
	SystemVueModelBuilder::IntMatrix outQ;

	outA.Resize(rows, cols);
	outI.Resize(rows, cols);
	outQ.Resize(rows, cols);

	for (std::size_t i = 0; i < elemCount; ++i)
	{
		std::complex<double> xd = apply_distortion_(sampled[i], t, distortionStates_[i]);

		QuantResult qi = quantize_(xd.real());
		QuantResult qq = quantize_(xd.imag());

		outA(i) = std::complex<double>(qi.analog, qq.analog);
		outI(i) = qi.codeDigital;
		outQ(i) = qq.codeDigital;

		distortionStates_[i].lastInput = sampled[i];
		distortionStates_[i].lastInputTime = t;
		distortionStates_[i].hasLastInput = true;
	}

	A_out[0] = outA;
	D_I[0] = outI;
	D_Q[0] = outQ;

	++sampleIndex_;

	return true;
}


// -----------------------------------------------------------------------------
// 参数处理
// -----------------------------------------------------------------------------

void AtoD_M::clamp_params_()
{
	nbits_ = clamp_int_(NBits, 4, 16);

	if (VRef <= 0.0)
		vref_ = 1.0;
	else
		vref_ = VRef;

	codeCount_ = 1 << nbits_;
	midCode_ = codeCount_ / 2;
	lsb_ = 2.0 * vref_ / static_cast<double>(codeCount_);

	if (DownsampleFactor < 1)
		DownsampleFactor = 1;

	if (DownsamplePhase < 0)
		DownsamplePhase = 0;

	if (DownsamplePhase >= DownsampleFactor)
		DownsamplePhase = DownsampleFactor - 1;

	if (Clock <= 0.0)
		Clock = 0.2e6;

	if (RJrms < 0.0)
		RJrms = 0.0;

	if (DNL < 0.0)
		DNL = 0.0;

	// 内置行为：DNL=0 时，INL 会被强制为 0。
	if (DNL <= 0.0)
		INL = 0.0;

	if (INL < DNL / 2.0)
		INL = DNL / 2.0;

	if (ENOB < 1.0)  ENOB = 1.0;
	if (ENOB > 16.0) ENOB = 16.0;

	ExcessBW = clip_(ExcessBW, 0.0, 1.0);
}


void AtoD_M::build_transfer_table_()
{
	thresholds_.assign(codeCount_ + 1, 0.0);
	levels_.assign(codeCount_, 0.0);

	thresholds_[0] = -vref_;
	thresholds_[codeCount_] = vref_;

	const bool useNonlinear =
		(DistortionModel == Jitter_INL_DNL) &&
		(DNL > 0.0 || INL > 0.0);

	if (!useNonlinear)
	{
		for (int i = 1; i < codeCount_; ++i)
			thresholds_[i] = -vref_ + static_cast<double>(i) * lsb_;

		for (int i = 0; i < codeCount_; ++i)
			levels_[i] = -vref_ + (static_cast<double>(i) + 0.5) * lsb_;

		return;
	}

	// 近似内置 INL/DNL 随机传递曲线；该部分只做趋势近似。
	std::vector<double> widths(codeCount_, lsb_);

	unsigned int local = 0xA5A5A5A5U ^ static_cast<unsigned int>(nbits_ * 131U);

	for (int i = 0; i < codeCount_; ++i)
	{
		local ^= (local << 13);
		local ^= (local >> 17);
		local ^= (local << 5);

		double u = (static_cast<double>(local) + 1.0) / 4294967297.0;
		double e = (2.0 * u - 1.0) * DNL;

		e = clip_(e, -0.95, 0.95);
		widths[i] = lsb_ * (1.0 + e);
	}

	double sumW = 0.0;
	for (int i = 0; i < codeCount_; ++i)
		sumW += widths[i];

	double scale = (2.0 * vref_) / std::max(sumW, kTiny);

	thresholds_[0] = -vref_;
	for (int i = 1; i < codeCount_; ++i)
		thresholds_[i] = thresholds_[i - 1] + widths[i - 1] * scale;
	thresholds_[codeCount_] = vref_;

	for (int i = 0; i < codeCount_; ++i)
	{
		double center = 0.5 * (thresholds_[i] + thresholds_[i + 1]);

		double u = static_cast<double>(i) / static_cast<double>(std::max(codeCount_ - 1, 1));
		double inlOffset = INL * lsb_ * std::sin(2.0 * kPi * u);

		levels_[i] = clip_(center + inlOffset, -vref_ + 0.5 * lsb_, vref_ - 0.5 * lsb_);
	}
}


void AtoD_M::reset_states_()
{
	clockStates_.clear();
	distortionStates_.clear();
}


void AtoD_M::ensure_state_count_(std::size_t n)
{
	if (clockStates_.size() != n)
		clockStates_.assign(n, ClockState());

	if (distortionStates_.size() != n)
		distortionStates_.assign(n, DistortionState());
}


// -----------------------------------------------------------------------------
// 输入采样
// -----------------------------------------------------------------------------

std::complex<double> AtoD_M::read_matrix_sample_(int idx, std::size_t elem)
{
	const unsigned int uidx = static_cast<unsigned int>(idx < 0 ? 0 : idx);
    const SystemVueModelBuilder::EnvelopeMatrix& m = A_Input[uidx];

	if (elem >= m.NumElements())
		return std::complex<double>(0.0, 0.0);

	return m(elem).complex();
}


std::vector<std::complex<double> > AtoD_M::get_downsampled_matrix_(std::size_t elemCount)
{
	std::vector<std::complex<double> > out(elemCount, std::complex<double>(0.0, 0.0));

	int factor = std::max(1, DownsampleFactor);
	int phase = clamp_int_(DownsamplePhase, 0, factor - 1);

	if (AntiAliasingFilter != AA_ON || factor <= 1)
	{
		for (std::size_t i = 0; i < elemCount; ++i)
			out[i] = read_matrix_sample_(phase, i);
		return out;
	}

	// 简化的抗混叠近似：
	// 内置为 raised-cosine 低通，这里用带余弦窗的加权平均近似。
	std::vector<std::complex<double> > acc(elemCount, std::complex<double>(0.0, 0.0));
	double wsum = 0.0;

	for (int k = 0; k < factor; ++k)
	{
		double x = 0.0;
		if (factor > 1)
			x = static_cast<double>(k) / static_cast<double>(factor - 1);

		double w = 0.5 - 0.5 * std::cos(2.0 * kPi * x);
		w = (1.0 - ExcessBW) + ExcessBW * w;

		for (std::size_t i = 0; i < elemCount; ++i)
			acc[i] += w * read_matrix_sample_(k, i);

		wsum += w;
	}

	if (wsum <= kTiny)
	{
		for (std::size_t i = 0; i < elemCount; ++i)
			out[i] = read_matrix_sample_(phase, i);
		return out;
	}

	for (std::size_t i = 0; i < elemCount; ++i)
		out[i] = acc[i] / wsum;

	return out;
}


double AtoD_M::first_positive_crossing_at_or_after_(double t) const
{
	// 内置文档：Clocked 模式使用内部余弦时钟的 positive zero crossing 采样。
	// cos(wt + phi) 从负到正过零对应 wt + phi = 3*pi/2 + 2*pi*k。
	const double period = 1.0 / std::max(Clock, kTiny);
	const double phaseRad = Phase * kPi / 180.0;
	const double base = ((1.5 * kPi) - phaseRad) / (2.0 * kPi * std::max(Clock, kTiny));

	double k = std::ceil((t - base) / period - 1e-12);
	if (k < 0.0)
		k = 0.0;

	double ts = base + k * period;
	while (ts < t - 1e-15)
		ts += period;

	return ts;
}


std::complex<double> AtoD_M::interp_(const std::complex<double>& x0,
	double t0,
	const std::complex<double>& x1,
	double t1,
	double ts) const
{
	if (std::fabs(t1 - t0) <= kTiny)
		return x1;

	double a = (ts - t0) / (t1 - t0);
	a = clip_(a, 0.0, 1.0);
	return x0 + (x1 - x0) * a;
}


std::complex<double> AtoD_M::get_clocked_input_(const std::complex<double>& x, double t, ClockState& st)
{
	if (Clock <= 0.0)
		return x;

	const double phaseRad = Phase * kPi / 180.0;
	const double c = std::cos(2.0 * kPi * Clock * t + phaseRad);
	const double period = 1.0 / std::max(Clock, kTiny);

	if (!st.hasClockState)
	{
		st.heldSample = std::complex<double>(0.0, 0.0);
		st.hasPendingClockSample = false;
		st.pendingClockSample = std::complex<double>(0.0, 0.0);

		st.lastClockValue = c;
		st.hasClockState = true;

		st.hasRawInputState = true;
		st.prevRawInputTime = t;
		st.prevRawInput = x;

		st.nextClockCrossingTime = first_positive_crossing_at_or_after_(t);
		st.hasNextClockCrossing = true;

		if (st.nextClockCrossingTime <= t + 1e-15)
		{
			st.pendingClockSample = x;
			st.hasPendingClockSample = true;
			st.nextClockCrossingTime += period;
		}

		return st.heldSample;
	}

	if (st.hasPendingClockSample)
	{
		st.heldSample = st.pendingClockSample;
		st.hasPendingClockSample = false;
	}

	const std::complex<double> y = st.heldSample;

	if (!st.hasRawInputState)
	{
		st.hasRawInputState = true;
		st.prevRawInputTime = t;
		st.prevRawInput = x;
		st.lastClockValue = c;
		return y;
	}

	if (t < st.prevRawInputTime - 1e-15)
	{
		st.prevRawInputTime = t;
		st.prevRawInput = x;
		st.nextClockCrossingTime = first_positive_crossing_at_or_after_(t);
		st.hasNextClockCrossing = true;
		st.lastClockValue = c;
		return y;
	}

	if (!st.hasNextClockCrossing)
	{
		st.nextClockCrossingTime = first_positive_crossing_at_or_after_(st.prevRawInputTime);
		st.hasNextClockCrossing = true;
	}

	while (st.nextClockCrossingTime <= t + 1e-15)
	{
		if (st.nextClockCrossingTime >= st.prevRawInputTime - 1e-15)
		{
			st.pendingClockSample = interp_(st.prevRawInput, st.prevRawInputTime, x, t, st.nextClockCrossingTime);
			st.hasPendingClockSample = true;
		}
		st.nextClockCrossingTime += period;
	}

	st.prevRawInputTime = t;
	st.prevRawInput = x;
	st.lastClockValue = c;

	return y;
}


// -----------------------------------------------------------------------------
// 失真模型
// -----------------------------------------------------------------------------

std::complex<double> AtoD_M::apply_distortion_(const std::complex<double>& x, double t, DistortionState& st)
{
	std::complex<double> y = x;

	if (DistortionModel == Distortion_None)
		return y;

	if (DistortionModel == Jitter_INL_DNL)
	{
		if (EnableJitter == Time_Domain)
			y = apply_jitter_(y, t, st);
		else if (EnableJitter == Frequency_Domain)
			y = apply_phase_noise_(y);

		return y;
	}

	if (DistortionModel == ENOB_value)
	{
		y = apply_noise_(y, target_snr_db_());
		return y;
	}

	if (DistortionModel == SNR_and_Harmonics)
	{
		y = apply_harmonics_(y, H2_dBc, H3_dBc, H4_dBc, H5_dBc);
		y = apply_noise_(y, target_snr_db_());
		return y;
	}

	if (DistortionModel == SINAD_and_SFDR)
	{
		y = apply_sinad_sfdr_(y);
		y = apply_noise_(y, target_snr_db_());
		return y;
	}

	return y;
}


std::complex<double> AtoD_M::apply_jitter_(const std::complex<double>& x, double t, DistortionState& st)
{
	if (RJrms <= 0.0 || !st.hasLastInput)
		return x;

	double dt = t - st.lastInputTime;
	if (std::fabs(dt) <= kTiny)
		return x;

	double jitter = RJrms * gaussian_();
	jitter = clip_(jitter, -3.0 * RJrms, 3.0 * RJrms);

	std::complex<double> slope = (x - st.lastInput) / dt;
	return x + slope * jitter;
}


std::complex<double> AtoD_M::apply_phase_noise_(const std::complex<double>& x)
{
	double sigma = 0.0;

	if (PhaseNoiseData != nullptr && PhaseNoiseDataSize >= 2)
	{
		double acc = 0.0;
		int pairs = PhaseNoiseDataSize / 2;

		for (int i = 0; i < pairs; ++i)
		{
			double ldbc = PhaseNoiseData[2 * i + 1];
			acc += std::pow(10.0, ldbc / 10.0);
		}

		sigma = std::sqrt(std::max(acc, 0.0)) * 1e-3;
	}
	else
	{
		sigma = 0.0;
	}

	if (sigma <= 0.0)
		return x;

	double ph = sigma * gaussian_();

	if (PN_Type == Fixed_freq_offset)
		ph = sigma;
	else if (PN_Type == Fixed_freq_offset_and_amplitude)
		ph = sigma * std::sin(2.0 * kPi * static_cast<double>(sampleIndex_) / 1024.0);

	return x * std::complex<double>(std::cos(ph), std::sin(ph));
}


std::complex<double> AtoD_M::apply_harmonics_(const std::complex<double>& x,
	double h2_dBc,
	double h3_dBc,
	double h4_dBc,
	double h5_dBc) const
{
	double i = apply_harmonics_real_(x.real(), h2_dBc, h3_dBc, h4_dBc, h5_dBc);
	double q = apply_harmonics_real_(x.imag(), h2_dBc, h3_dBc, h4_dBc, h5_dBc);

	return std::complex<double>(i, q);
}


std::complex<double> AtoD_M::apply_sinad_sfdr_(const std::complex<double>& x) const
{
	double sfdr = SFDR_dBc;

	if (sfdr < SINAD_dB + 6.0)
		sfdr = SINAD_dB + 6.0;

	if (sfdr > SINAD_dB + 20.0)
		return x;

	double h2 = -std::fabs(sfdr);
	return apply_harmonics_(x, h2, h2 - 3.0, -400.0, -400.0);
}


double AtoD_M::apply_harmonics_real_(double x,
	double h2_dBc,
	double h3_dBc,
	double h4_dBc,
	double h5_dBc) const
{
	double A = std::max(full_scale_peak_(), std::fabs(x));
	A = std::max(A, kTiny);

	double u = clip_(x / A, -1.0, 1.0);

	double T2 = 2.0 * u * u - 1.0;
	double T3 = 4.0 * u * u * u - 3.0 * u;
	double u2 = u * u;
	double u3 = u2 * u;
	double u4 = u2 * u2;
	double u5 = u4 * u;
	double T4 = 8.0 * u4 - 8.0 * u2 + 1.0;
	double T5 = 16.0 * u5 - 20.0 * u3 + 5.0 * u;

	double y = x;

	if (h2_dBc > -300.0) y += A * db_to_amp_(h2_dBc) * T2;
	if (h3_dBc > -300.0) y += A * db_to_amp_(h3_dBc) * T3;
	if (h4_dBc > -300.0) y += A * db_to_amp_(h4_dBc) * T4;
	if (h5_dBc > -300.0) y += A * db_to_amp_(h5_dBc) * T5;

	return y;
}


std::complex<double> AtoD_M::apply_noise_(const std::complex<double>& x, double snr_dB)
{
	if (snr_dB > 250.0)
		return x;

	double sigRms = std::max(std::abs(x) / std::sqrt(2.0),
		(vref_ / std::sqrt(2.0)) * db_to_amp_(Level_dBFS));

	double noiseRms = sigRms * db_to_amp_(-snr_dB);

	if (noiseRms <= 0.0)
		return x;

	double ni = noiseRms * gaussian_();
	double nq = noiseRms * gaussian_();

	return x + std::complex<double>(ni, nq);
}


// -----------------------------------------------------------------------------
// 量化
// -----------------------------------------------------------------------------

AtoD_M::QuantResult AtoD_M::quantize_(double x) const
{
	QuantResult r;

	double xc = clip_(x, -vref_, vref_);

	const bool idealUniformTable =
		!(DistortionModel == Jitter_INL_DNL && (DNL > 0.0 || INL > 0.0));

	if (idealUniformTable && !thresholds_.empty())
	{
		// 继承当前 AtoD 差异较小版本中的边界吸附策略。
		const double edgeTol = lsb_ * 5.0e-3;

		for (int k = 1; k < codeCount_; ++k)
		{
			const double th = thresholds_[k];
			if (xc < th && (th - xc) <= edgeTol)
			{
				xc = th;
				break;
			}
		}
	}

	int code = 0;

	if (thresholds_.empty() || levels_.empty())
	{
		double u = (xc + vref_) / lsb_;
		code = static_cast<int>(std::floor(u));
		code = clamp_int_(code, 0, codeCount_ - 1);

		r.analog = -vref_ + (static_cast<double>(code) + 0.5) * lsb_;
	}
	else
	{
		if (xc <= thresholds_.front())
		{
			code = 0;
		}
		else if (xc >= thresholds_.back())
		{
			code = codeCount_ - 1;
		}
		else
		{
			std::vector<double>::const_iterator it =
				std::upper_bound(thresholds_.begin(), thresholds_.end(), xc);

			code = static_cast<int>((it - thresholds_.begin()) - 1);
			code = clamp_int_(code, 0, codeCount_ - 1);
		}

		r.analog = levels_[code];
	}

	r.codeOffset = code;

	if (OutputDigitalFormat == Twos_complement)
		r.codeDigital = code - midCode_;
	else
		r.codeDigital = code;

	return r;
}


// -----------------------------------------------------------------------------
// SNR / 工具函数
// -----------------------------------------------------------------------------

double AtoD_M::target_snr_db_() const
{
	double idealSNR = 6.02 * static_cast<double>(nbits_) + 1.76;

	if (DistortionModel == ENOB_value)
	{
		double snr = 6.02 * ENOB + 1.76;

		if (snr > idealSNR + std::fabs(Level_dBFS))
			snr = idealSNR + std::fabs(Level_dBFS);

		return snr;
	}

	if (DistortionModel == SNR_and_Harmonics)
	{
		double snr = SNR_dB;
		if (snr > idealSNR + std::fabs(Level_dBFS))
			snr = idealSNR + std::fabs(Level_dBFS);
		return snr;
	}

	if (DistortionModel == SINAD_and_SFDR)
	{
		double snr = SINAD_dB;
		if (snr > idealSNR + std::fabs(Level_dBFS))
			snr = idealSNR + std::fabs(Level_dBFS);
		return snr;
	}

	return 300.0;
}


double AtoD_M::full_scale_peak_() const
{
	return std::max(vref_ * db_to_amp_(Level_dBFS), kTiny);
}


double AtoD_M::uniform_()
{
	rngState_ ^= (rngState_ << 13);
	rngState_ ^= (rngState_ >> 17);
	rngState_ ^= (rngState_ << 5);

	return (static_cast<double>(rngState_) + 1.0) / 4294967297.0;
}


double AtoD_M::gaussian_()
{
	double u1 = std::max(uniform_(), 1e-12);
	double u2 = std::max(uniform_(), 1e-12);

	double r = std::sqrt(-2.0 * std::log(u1));
	double th = 2.0 * kPi * u2;

	return r * std::cos(th);
}


double AtoD_M::db_to_amp_(double dB)
{
	return std::pow(10.0, dB / 20.0);
}


double AtoD_M::clip_(double x, double lo, double hi)
{
	if (x < lo) return lo;
	if (x > hi) return hi;
	return x;
}


int AtoD_M::clamp_int_(int v, int lo, int hi)
{
	if (v < lo) return lo;
	if (v > hi) return hi;
	return v;
}
