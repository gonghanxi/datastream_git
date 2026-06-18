#include "AtoD.h"

#include <algorithm>
#include <ctime>
#include <cstdlib>

const double AtoD::kPi = 3.1415926535897932384626433832795;
const double AtoD::kTiny = 1e-30;

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(AtoD)
{
	SET_MODEL_DESCRIPTION("Analog to Digital Converter");
	SET_MODEL_SYMBOL("SYM_AtoD");
	SET_MODEL_CATEGORY("Algorithm Design");

	// --------- 端口 ---------
	{
		SystemVueModelBuilder::DFPort p = ADD_MODEL_INPUT(A_in);
		p.SetDescription("input analog signal");
	}
	{
		SystemVueModelBuilder::DFPort p = ADD_MODEL_OUTPUT(A_out);
		p.SetDescription("output sampled analog baseband signal");
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
		SystemVueModelBuilder::DFParam p = ADD_MODEL_ENUM_PARAM(ADCType, ADCTypeEnum);
		p.AddEnumeration("Current AtoD", AtoD::Current_AtoD);
		p.AddEnumeration("Flash", AtoD::Flash_ADC);
		p.AddEnumeration("Pipeline", AtoD::Pipeline_ADC);
		p.AddEnumeration("SigmaDelta", AtoD::SigmaDelta_ADC);
		p.SetDefaultValue("Current AtoD");
		p.SetDescription("ADC architecture type: Current AtoD, Flash, Pipeline, SigmaDelta");
	}
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_ENUM_PARAM(OutputDigitalFormat, OutputDigitalFormatEnum);
		p.AddEnumeration("Offset binary", AtoD::Offset_binary);
		p.AddEnumeration("Twos-complement", AtoD::Twos_complement);
		p.SetDefaultValue("Offset binary");
		p.SetDescription("Output digital format: Offset binary, Twos-complement");
	}
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_ENUM_PARAM(DistortionModel, DistortionModelEnum);
		p.AddEnumeration("None", AtoD::Distortion_None);
		p.AddEnumeration("Jitter/INL/DNL", AtoD::Jitter_INL_DNL);
		p.AddEnumeration("ENOB value", AtoD::ENOB_value);
		p.AddEnumeration("SNR and Harmonics", AtoD::SNR_and_Harmonics);
		p.AddEnumeration("SINAD and SFDR", AtoD::SINAD_and_SFDR);
		p.SetDefaultValue("Jitter/INL/DNL");
		p.SetDescription("Distortion model: None, Jitter/INL/DNL, ENOB value, SNR and Harmonics, SINAD and SFDR");
		p.SetHideCondition("ADCType ~= 0");
	}

	// --------- Jitter / INL / DNL 参数 ---------
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_ENUM_PARAM(EnableJitter, EnableJitterEnum);
		p.AddEnumeration("No", AtoD::Jitter_No);
		p.AddEnumeration("Time Domain", AtoD::Time_Domain);
		p.AddEnumeration("Frequency Domain", AtoD::Frequency_Domain);
		p.SetDefaultValue("No");
		p.SetDescription("Enable jitter: No, Time Domain, Frequency Domain");
		p.SetHideCondition("ADCType ~= 0 || DistortionModel ~= 1");
	}
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(RJrms);
		p.SetDefaultValue("0.0");
		p.SetUnit(SystemVueModelBuilder::Units::TIME);
		p.SetDescription("Random jitter standard deviation");
		p.SetHideCondition("ADCType ~= 0 || DistortionModel ~= 1 || EnableJitter ~= 1");
	}
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_ARRAY_PARAM(PhaseNoiseData, PhaseNoiseDataSize);
		p.SetDefaultValue("");
		p.SetDescription("Phase noise specification - pairs of offset freq (Hz) and SSB phase noise level (dBc/Hz)");
		p.SetHideCondition("ADCType ~= 0 || DistortionModel ~= 1 || EnableJitter ~= 2");
	}
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_ENUM_PARAM(PN_Type, PN_TypeEnum);
		p.AddEnumeration("Random PN", AtoD::Random_PN);
		p.AddEnumeration("Fixed freq offset", AtoD::Fixed_freq_offset);
		p.AddEnumeration("Fixed freq offset and amplitude", AtoD::Fixed_freq_offset_and_amplitude);
		p.SetDefaultValue("Random PN");
		p.SetDescription("Phase noise model type with random or fixed offset freq spacing and amplitude: Random PN, Fixed freq offset, Fixed freq offset and amplitude");
		p.SetHideCondition("ADCType ~= 0 || DistortionModel ~= 1 || EnableJitter ~= 2");
	}
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(INL);
		p.SetDefaultValue("0.0");
		p.SetDescription("Integral nonlinearity relative to least significant bit (LSB)");
		p.SetHideCondition("ADCType ~= 0 || DistortionModel ~= 1");
	}
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(DNL);
		p.SetDefaultValue("0.0");
		p.SetDescription("Differential nonlinearity relative to least significant bit (LSB)");
		p.SetHideCondition("ADCType ~= 0 || DistortionModel ~= 1");
	}

	// --------- ENOB 参数 ---------
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(ENOB);
		p.SetDefaultValue("7");
		p.SetDescription("Equivalent number of bits (based on INL and DNL)");
		p.SetHideCondition("ADCType ~= 0 || DistortionModel ~= 2");
	}

	// --------- SNR and Harmonics 参数 ---------
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(SNR_dB);
		p.SetDefaultValue("60.0");
		p.SetDescription("SNR output in dB for analog input");
		p.SetHideCondition("ADCType ~= 0 || DistortionModel ~= 3");
	}
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(H2_dBc);
		p.SetDefaultValue("-400.0");
		p.SetDescription("2nd harmonic output level in dBc relative to fundamental output");
		p.SetHideCondition("ADCType ~= 0 || DistortionModel ~= 3");
	}
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(H3_dBc);
		p.SetDefaultValue("-400.0");
		p.SetDescription("3rd harmonic output level in dBc relative to fundamental output");
		p.SetHideCondition("ADCType ~= 0 || DistortionModel ~= 3");
	}
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(H4_dBc);
		p.SetDefaultValue("-400.0");
		p.SetDescription("4th harmonic output level in dBc relative to fundamental output");
		p.SetHideCondition("ADCType ~= 0 || DistortionModel ~= 3");
	}
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(H5_dBc);
		p.SetDefaultValue("-400.0");
		p.SetDescription("5th harmonic output level in dBc relative to fundamental output");
		p.SetHideCondition("ADCType ~= 0 || DistortionModel ~= 3");
	}

	// --------- SINAD and SFDR 参数 ---------
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(SINAD_dB);
		p.SetDefaultValue("60.0");
		p.SetDescription("Output signal to (noise plus harmonic distortion) ratio in dB");
		p.SetHideCondition("ADCType ~= 0 || DistortionModel ~= 4");
	}
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(SFDR_dBc);
		p.SetDefaultValue("70.0");
		p.SetDescription("Output spurious free dynamic range in dBc relative to fundamental output level");
		p.SetHideCondition("ADCType ~= 0 || DistortionModel ~= 4");
	}
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_ENUM_PARAM(FFT_Size, FFT_SizeEnum);
		p.AddEnumeration("2^12", AtoD::FFT_2_12);
		p.AddEnumeration("2^13", AtoD::FFT_2_13);
		p.AddEnumeration("2^14", AtoD::FFT_2_14);
		p.AddEnumeration("2^15", AtoD::FFT_2_15);
		p.AddEnumeration("2^16", AtoD::FFT_2_16);
		p.SetDefaultValue("2^14");
		p.SetDescription("FFT size as power of 2: 2^12, 2^13, 2^14, 2^15, 2^16");
		p.SetHideCondition("ADCType ~= 0 || DistortionModel ~= 4");
	}

	// --------- SNR_Model 及其相关参数 ---------
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_ENUM_PARAM(SNR_Model, SNR_ModelEnum);
		p.AddEnumeration("Quantization_and_Jitter", AtoD::Quantization_and_Jitter);
		p.AddEnumeration("Quantization_and_INL_DNL", AtoD::Quantization_and_INL_DNL);
		p.AddEnumeration("Quantization_and_Jitter_or_INL_DNL", AtoD::Quantization_and_Jitter_or_INL_DNL);
		p.AddEnumeration("Quantization_Jitter_and_Thermal_Noise", AtoD::Quantization_Jitter_and_Thermal_Noise);
		p.SetDefaultValue("Quantization_and_Jitter");
		p.SetDescription("SNR model: Quantization_and_Jitter, Quantization_and_INL_DNL, Quantization_and_Jitter_or_INL_DNL, Quantization_Jitter_and_Thermal_Noise");
		p.SetHideCondition("ADCType ~= 0 || DistortionModel == 0 || DistortionModel == 1");
	}
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(ThermalNoise_SNR_dBFS);
		p.SetDefaultValue("63");
		p.SetDescription("Thermal noise level in dBFS");
		p.SetHideCondition("ADCType ~= 0 || (DistortionModel == 0 || DistortionModel == 1) || SNR_Model ~= 3");
	}
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(CenterFreq);
		p.SetDefaultValue("100.0e6");
		p.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		p.SetDescription("Spectral center frequency for analog input");
		p.SetHideCondition("ADCType ~= 0 || (DistortionModel == 0 || DistortionModel == 1) || SNR_Model == 1");
	}
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(Level_dBFS);
		p.SetDefaultValue("0.0");
		p.SetDescription("Signal level in dBFS for analog input");
		p.SetHideCondition("ADCType ~= 0 || DistortionModel == 0 || DistortionModel == 1");
	}

	// --------- 转换方式参数 ---------
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_ENUM_PARAM(ConversionType, ConversionTypeEnum);
		p.AddEnumeration("Clocked", AtoD::Clocked);
		p.AddEnumeration("Downsampled", AtoD::Downsampled);
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
		p.SetUnit(SystemVueModelBuilder::Units::ANGLE);
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
		p.SetDescription("Downsampling phase");
		p.SetHideCondition("ConversionType ~= 1");
	}
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_ENUM_PARAM(AntiAliasingFilter, AntiAliasingFilterEnum);
		p.AddEnumeration("OFF", AtoD::AA_OFF);
		p.AddEnumeration("ON", AtoD::AA_ON);
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

	// --------- 新增 ADC 架构基础参数 ---------
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(PipelineStageBits);
		p.SetDefaultValue("1");
		p.SetDescription("Pipeline ADC stage bits for basic architecture model");
		p.SetHideCondition("ADCType ~= 2");
	}
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(PipelineLatency);
		p.SetDefaultValue("0");
		p.SetDescription("Pipeline ADC output latency in samples");
		p.SetHideCondition("ADCType ~= 2");
	}
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(SigmaDeltaOrder);
		p.SetDefaultValue("1");
		p.SetDescription("SigmaDelta ADC order; this basic model implements first-order behavior");
		p.SetHideCondition("ADCType ~= 3");
	}
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(SigmaDeltaOSR);
		p.SetDefaultValue("16");
		p.SetDescription("SigmaDelta ADC oversampling ratio for moving-average decimation approximation");
		p.SetHideCondition("ADCType ~= 3");
	}

	return true;
}
#endif


AtoD::AtoD() : NBits(8),
VRef(1.0),
ADCType(Current_AtoD),
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
PipelineStageBits(1),
PipelineLatency(0),
SigmaDeltaOrder(1),
SigmaDeltaOSR(16),
nbits_(8),
codeCount_(256),
midCode_(128),
vref_(1.0),
lsb_(2.0 / 256.0),
sampleIndex_(0ULL),
hasClockState_(false),
lastClockValue_(0.0),
heldSample_(0.0, 0.0),
hasPendingClockSample_(false),
pendingClockSample_(0.0, 0.0),
hasRawInputState_(false),
prevRawInputTime_(0.0),
prevRawInput_(0.0, 0.0),
hasNextClockCrossing_(false),
nextClockCrossingTime_(0.0),
hasLastInput_(false),
lastInputTime_(0.0),
lastInput_(0.0, 0.0),
rngState_(0x12345678U),
sdIIntegrator_(0.0),
sdQIntegrator_(0.0),
sdIFeedback_(0.0),
sdQFeedback_(0.0),
sdIAccum_(0.0),
sdQAccum_(0.0),
sdAccumCount_(0),
sdHeldOutput_(0.0, 0.0)
{
}


ERESULT AtoD::PropagateCharacterizationFrequency()
{
	A_out.SetCharacterizationFrequency(A_in.GetCharacterizationFrequency());
	return true;
}


bool AtoD::Setup()
{
	clamp_params_();

	// Downsampled 模式下，一次 Run 读取 DownsampleFactor 个输入样本，输出 1 个样本。
	if (ConversionType == Downsampled)
	{
		try_set_rate_(A_in, DownsampleFactor, 0);
	}
	else
	{
		try_set_rate_(A_in, 1, 0);
	}

	try_set_rate_(A_out, 1, 0);
	try_set_rate_(D_I, 1, 0);
	try_set_rate_(D_Q, 1, 0);

	build_transfer_table_();

	sampleIndex_ = 0ULL;
	hasClockState_ = false;
	lastClockValue_ = 0.0;
	heldSample_ = std::complex<double>(0.0, 0.0);
	hasPendingClockSample_ = false;
	pendingClockSample_ = std::complex<double>(0.0, 0.0);

	hasRawInputState_ = false;
	prevRawInputTime_ = 0.0;
	prevRawInput_ = std::complex<double>(0.0, 0.0);
	hasNextClockCrossing_ = false;
	nextClockCrossingTime_ = 0.0;

	hasLastInput_ = false;
	lastInputTime_ = 0.0;
	lastInput_ = std::complex<double>(0.0, 0.0);

	// 随机模型使用非固定初值，使 Time Domain Jitter / ENOB / SNR 噪声每次运行可变化。
	// 这更接近内置随机模型，但不保证逐点一致。
	rngState_ = static_cast<unsigned int>(std::time(nullptr)) ^ 0x9E3779B9U;

	pipelineFifo_.clear();

	sdIIntegrator_ = 0.0;
	sdQIntegrator_ = 0.0;
	sdIFeedback_ = 0.0;
	sdQFeedback_ = 0.0;
	sdIAccum_ = 0.0;
	sdQAccum_ = 0.0;
	sdAccumCount_ = 0;
	sdHeldOutput_ = std::complex<double>(0.0, 0.0);

	A_out.SetCharacterizationFrequency(A_in.GetCharacterizationFrequency());

	return true;
}


bool AtoD::Run()
{
	double t = A_out.GetTime(0, GetCount());

	std::complex<double> x;

	if (ConversionType == Downsampled)
	{
		x = get_downsampled_input_();
	}
	else
	{
		std::complex<double> xin = read_input_sample_(0);
		x = get_clocked_input_(xin, t);
	}

	QuantResult qi;
	QuantResult qq;

	if (ADCType == Flash_ADC)
	{
		// Flash 型：基础比较器阵列量化，不叠加 Current AtoD 的随机/频谱失真。
		qi = quantize_flash_(x.real());
		qq = quantize_flash_(x.imag());
	}
	else if (ADCType == Pipeline_ADC)
	{
		// Pipeline 型：基础模型为采样后经流水延迟，再进入理想 NBit 量化。
		std::complex<double> xp = process_pipeline_(x);
		qi = quantize_(xp.real());
		qq = quantize_(xp.imag());
	}
	else if (ADCType == SigmaDelta_ADC)
	{
		// SigmaDelta 型：基础模型为一阶 1-bit 调制器 + OSR 移动平均近似，再映射到 NBit 码。
		std::complex<double> xs = process_sigma_delta_(x);
		qi = quantize_(xs.real());
		qq = quantize_(xs.imag());
	}
	else
	{
		// Current AtoD：完全保留原有最接近 SystemVue 内置 AtoD 的路径。
		std::complex<double> xd = apply_distortion_(x, t);
		qi = quantize_(xd.real());
		qq = quantize_(xd.imag());
	}

	A_out[0] = std::complex<double>(qi.analog, qq.analog);
	D_I[0] = qi.codeDigital;
	D_Q[0] = qq.codeDigital;

	lastInput_ = x;
	lastInputTime_ = t;
	hasLastInput_ = true;

	++sampleIndex_;

	return true;
}


// -----------------------------------------------------------------------------
// 参数处理
// -----------------------------------------------------------------------------

void AtoD::clamp_params_()
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

	PipelineStageBits = clamp_int_(PipelineStageBits, 1, std::max(1, nbits_));
	if (PipelineLatency < 0)
		PipelineLatency = 0;

	// 当前基础 SigmaDelta 只实现一阶行为；参数保留是为了后续扩展。
	if (SigmaDeltaOrder < 1)
		SigmaDeltaOrder = 1;
	if (SigmaDeltaOSR < 1)
		SigmaDeltaOSR = 1;
}



void AtoD::build_transfer_table_()
{
	thresholds_.assign(codeCount_ + 1, 0.0);
	levels_.assign(codeCount_, 0.0);

	thresholds_[0] = -vref_;
	thresholds_[codeCount_] = vref_;

	const bool useNonlinear =
		(ADCType == Current_AtoD) &&
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

	// 近似内置的 INL/DNL 随机传递曲线：
	// 1. 用固定伪随机序列生成每级码宽扰动；
	// 2. 将总跨度归一化到 2*VRef；
	// 3. 码中心由相邻阈值中点给出；
	// 4. INL 用低频累计形状近似。
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

		// INL 低频弯曲近似，单位为 LSB。
		double u = static_cast<double>(i) / static_cast<double>(std::max(codeCount_ - 1, 1));
		double inlOffset = INL * lsb_ * std::sin(2.0 * kPi * u);

		levels_[i] = clip_(center + inlOffset, -vref_ + 0.5 * lsb_, vref_ - 0.5 * lsb_);
	}
}


// -----------------------------------------------------------------------------
// 输入采样
// -----------------------------------------------------------------------------

std::complex<double> AtoD::read_input_sample_(int idx)
{
	// EnvelopeCircularBuffer 的 operator[] 在 SystemVue 2020 中不是 const 接口，
	// 因此本函数不能声明为 const。
	// EnvelopeSignal 提供 complex()/real()/imag() 访问包络 I/Q 值。
	const unsigned int uidx = static_cast<unsigned int>(idx < 0 ? 0 : idx);
	return A_in[uidx].complex();
}


std::complex<double> AtoD::get_downsampled_input_()
{
	int factor = std::max(1, DownsampleFactor);
	int phase = clamp_int_(DownsamplePhase, 0, factor - 1);

	if (AntiAliasingFilter != AA_ON || factor <= 1)
	{
		return read_input_sample_(phase);
	}

	// 简化的抗混叠近似：
	// 内置为 raised-cosine 低通，这里用带余弦窗的加权平均近似。
	// 该部分用于趋势接近，不保证 AA=ON 时逐点对齐。
	std::complex<double> acc(0.0, 0.0);
	double wsum = 0.0;

	for (int i = 0; i < factor; ++i)
	{
		double x = 0.0;
		if (factor > 1)
			x = static_cast<double>(i) / static_cast<double>(factor - 1);

		double w = 0.5 - 0.5 * std::cos(2.0 * kPi * x);
		w = (1.0 - ExcessBW) + ExcessBW * w;

		acc += w * read_input_sample_(i);
		wsum += w;
	}

	if (wsum <= kTiny)
		return read_input_sample_(phase);

	return acc / wsum;
}


double AtoD::first_positive_crossing_at_or_after_(double t) const
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


std::complex<double> AtoD::interp_(const std::complex<double>& x0,
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


std::complex<double> AtoD::get_clocked_input_(const std::complex<double>& x, double t)
{
	if (Clock <= 0.0)
		return x;

	const double phaseRad = Phase * kPi / 180.0;
	const double c = std::cos(2.0 * kPi * Clock * t + phaseRad);
	const double period = 1.0 / std::max(Clock, kTiny);

	if (!hasClockState_)
	{
		// 内置 AtoD 在第一个 positive zero crossing 到来前保持 0 输入。
		// 注意：即使采样沿恰好落在首帧，也先登记 pending，当前帧仍输出旧保持值，
		// 这样才能避免每个采样沿提前一帧输出。
		heldSample_ = std::complex<double>(0.0, 0.0);
		hasPendingClockSample_ = false;
		pendingClockSample_ = std::complex<double>(0.0, 0.0);

		lastClockValue_ = c;
		hasClockState_ = true;

		hasRawInputState_ = true;
		prevRawInputTime_ = t;
		prevRawInput_ = x;

		nextClockCrossingTime_ = first_positive_crossing_at_or_after_(t);
		hasNextClockCrossing_ = true;

		if (nextClockCrossingTime_ <= t + 1e-15)
		{
			pendingClockSample_ = x;
			hasPendingClockSample_ = true;
			nextClockCrossingTime_ += period;
		}

		return heldSample_;
	}

	// 关键时序：上一帧检测到采样沿得到的新样本，本帧才进入保持输出。
	// 如果这里不延后一帧，C1/C2/J0/PN0 会表现为每隔一个时钟周期提前 1 点跳变。
	if (hasPendingClockSample_)
	{
		heldSample_ = pendingClockSample_;
		hasPendingClockSample_ = false;
	}

	const std::complex<double> y = heldSample_;

	if (!hasRawInputState_)
	{
		hasRawInputState_ = true;
		prevRawInputTime_ = t;
		prevRawInput_ = x;
		lastClockValue_ = c;
		return y;
	}

	// 如果仿真时间回退或重复初始化异常，重新建立时钟状态。
	if (t < prevRawInputTime_ - 1e-15)
	{
		prevRawInputTime_ = t;
		prevRawInput_ = x;
		nextClockCrossingTime_ = first_positive_crossing_at_or_after_(t);
		hasNextClockCrossing_ = true;
		lastClockValue_ = c;
		return y;
	}

	if (!hasNextClockCrossing_)
	{
		nextClockCrossingTime_ = first_positive_crossing_at_or_after_(prevRawInputTime_);
		hasNextClockCrossing_ = true;
	}

	// 内置更接近在真实 positive zero crossing 时刻采样，并对输入进行线性插值。
	// 但采样值不在当前 Run 立即输出，而是 pending 到下一帧。
	while (nextClockCrossingTime_ <= t + 1e-15)
	{
		if (nextClockCrossingTime_ >= prevRawInputTime_ - 1e-15)
		{
			pendingClockSample_ = interp_(prevRawInput_, prevRawInputTime_, x, t, nextClockCrossingTime_);
			hasPendingClockSample_ = true;
		}
		nextClockCrossingTime_ += period;
	}

	prevRawInputTime_ = t;
	prevRawInput_ = x;
	lastClockValue_ = c;

	return y;
}


// -----------------------------------------------------------------------------
// 失真模型
// -----------------------------------------------------------------------------

std::complex<double> AtoD::apply_distortion_(const std::complex<double>& x, double t)
{
	std::complex<double> y = x;

	if (DistortionModel == Distortion_None)
	{
		return y;
	}

	if (DistortionModel == Jitter_INL_DNL)
	{
		if (EnableJitter == Time_Domain)
			y = apply_jitter_(y, t);
		else if (EnableJitter == Frequency_Domain)
			y = apply_phase_noise_(y);

		// INL/DNL 已体现在 quantize_() 使用的 transfer table 中。
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


std::complex<double> AtoD::apply_jitter_(const std::complex<double>& x, double t)
{
	if (RJrms <= 0.0 || !hasLastInput_)
		return x;

	double dt = t - lastInputTime_;
	if (std::fabs(dt) <= kTiny)
		return x;

	double jitter = RJrms * gaussian_();

	// 内置文档说明随机抖动会限制在 ±3*RJrms。
	jitter = clip_(jitter, -3.0 * RJrms, 3.0 * RJrms);

	std::complex<double> slope = (x - lastInput_) / dt;
	return x + slope * jitter;
}


std::complex<double> AtoD::apply_phase_noise_(const std::complex<double>& x)
{
	// Frequency Domain PN 的严格实现需要相位噪声频谱合成。
	// 这里根据 PhaseNoiseData 粗略估算相位扰动强度。
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


std::complex<double> AtoD::apply_harmonics_(const std::complex<double>& x,
	double h2_dBc,
	double h3_dBc,
	double h4_dBc,
	double h5_dBc) const
{
	double i = apply_harmonics_real_(x.real(), h2_dBc, h3_dBc, h4_dBc, h5_dBc);
	double q = apply_harmonics_real_(x.imag(), h2_dBc, h3_dBc, h4_dBc, h5_dBc);

	return std::complex<double>(i, q);
}


std::complex<double> AtoD::apply_sinad_sfdr_(const std::complex<double>& x) const
{
	// 黑盒补测结论：
	// 1. SFDR_dBc < SINAD_dB + 6 时，内置会把 SFDR 抬到下限；
	// 2. SFDR_dBc 过大时，内置进入 no harmonics 分支；
	// 3. 合法区间内，最大 spur 与基波差值约等于 SFDR_dBc，常见为 2f0 spur。
	double sfdr = SFDR_dBc;

	if (sfdr < SINAD_dB + 6.0)
		sfdr = SINAD_dB + 6.0;

	// 经验近似：SFDR 比 SINAD 高太多时不生成离散谐波，只保留噪声贡献。
	if (sfdr > SINAD_dB + 20.0)
		return x;

	double h2 = -std::fabs(sfdr);
	return apply_harmonics_(x, h2, h2 - 3.0, -400.0, -400.0);
}


double AtoD::apply_harmonics_real_(double x,
	double h2_dBc,
	double h3_dBc,
	double h4_dBc,
	double h5_dBc) const
{
	double A = std::max(full_scale_peak_(), std::fabs(x));
	A = std::max(A, kTiny);

	double u = clip_(x / A, -1.0, 1.0);

	// Chebyshev 多项式：若 x=A*cos(wt)，Tn(x/A)=cos(nwt)。
	// 这样可直接生成目标 n 次谐波，避免普通 x^n 同时改变基波幅度。
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


std::complex<double> AtoD::apply_noise_(const std::complex<double>& x, double snr_dB)
{
	if (snr_dB > 250.0)
		return x;

	// 当前信号幅度过小时，用 Level_dBFS 推算的满幅正弦 RMS。
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

AtoD::QuantResult AtoD::quantize_(double x) const
{
	QuantResult r;

	double xc = clip_(x, -vref_, vref_);

	// ---------------------------------------------------------------------
	// SystemVue 内置 AtoD 的理想量化在部分 Clocked 插值点上表现为：
	// 当采样值非常接近某个量化门限、且由于双精度插值落在门限下方极小距离时，
	// 内置结果会进入上一级码。
	//
	// 之前只剩 C2/C3 少量点差 1 code，本质就是这个边界归属问题。
	// 这里仅在“理想均匀量化表”下，对门限下方极近处做向上吸附；
	// 不处理门限上方，也不在 INL/DNL 非线性表中启用，避免破坏已对齐的 C1、D1/D2、J0、PN0。
	// ---------------------------------------------------------------------
	const bool idealUniformTable =
		(ADCType != Current_AtoD) ||
		!(DistortionModel == Jitter_INL_DNL && (DNL > 0.0 || INL > 0.0));

	if (idealUniformTable && !thresholds_.empty())
	{
		// 1e-9 LSB 对当前 C2/C3 不够；这些点不是纯文本显示误差，
		// 而是 clock 插值与内置边界归属之间的微小偏差。
		// 0.5% LSB 只影响极靠近门限的样本，不会改变正常码区间内的值。
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



AtoD::QuantResult AtoD::quantize_flash_(double x) const
{
	QuantResult r;

	double xc = clip_(x, -vref_, vref_);
	int code = 0;

	// Flash ADC 基础模型：并行比较器阵列。
	// 第 k 个比较器阈值为 -VRef + k*LSB，输入高于或等于该阈值则计数加 1。
	for (int k = 1; k < codeCount_; ++k)
	{
		double th = -vref_ + static_cast<double>(k) * lsb_;
		if (xc >= th)
			++code;
		else
			break;
	}

	code = clamp_int_(code, 0, codeCount_ - 1);
	r.codeOffset = code;
	r.analog = -vref_ + (static_cast<double>(code) + 0.5) * lsb_;

	if (OutputDigitalFormat == Twos_complement)
		r.codeDigital = code - midCode_;
	else
		r.codeDigital = code;

	return r;
}


std::complex<double> AtoD::process_pipeline_(const std::complex<double>& x)
{
	// Pipeline ADC 基础模型：不改变幅度，仅模拟流水线输出延迟。
	// PipelineStageBits 作为架构参数保留，当前基础模型仍由最终 NBits 量化决定输出码。
	if (PipelineLatency <= 0)
		return x;

	pipelineFifo_.push_back(x);

	if (static_cast<int>(pipelineFifo_.size()) <= PipelineLatency)
		return std::complex<double>(0.0, 0.0);

	std::complex<double> y = pipelineFifo_.front();
	pipelineFifo_.erase(pipelineFifo_.begin());
	return y;
}


std::complex<double> AtoD::process_sigma_delta_(const std::complex<double>& x)
{
	// SigmaDelta ADC 基础模型：一阶 1-bit 调制器。
	// 输入先归一化到 [-1, 1]，调制输出 ±1，经 OSR 移动平均后恢复到电压域。
	double ui = clip_(x.real() / std::max(vref_, kTiny), -1.0, 1.0);
	double uq = clip_(x.imag() / std::max(vref_, kTiny), -1.0, 1.0);

	sdIIntegrator_ += ui - sdIFeedback_;
	double bi = (sdIIntegrator_ >= 0.0) ? 1.0 : -1.0;
	sdIFeedback_ = bi;

	sdQIntegrator_ += uq - sdQFeedback_;
	double bq = (sdQIntegrator_ >= 0.0) ? 1.0 : -1.0;
	sdQFeedback_ = bq;

	sdIAccum_ += bi;
	sdQAccum_ += bq;
	++sdAccumCount_;

	if (sdAccumCount_ >= std::max(1, SigmaDeltaOSR))
	{
		double ai = sdIAccum_ / static_cast<double>(sdAccumCount_);
		double aq = sdQAccum_ / static_cast<double>(sdAccumCount_);

		sdHeldOutput_ = std::complex<double>(clip_(ai * vref_, -vref_, vref_),
			clip_(aq * vref_, -vref_, vref_));

		sdIAccum_ = 0.0;
		sdQAccum_ = 0.0;
		sdAccumCount_ = 0;
	}
	else if (sampleIndex_ == 0ULL)
	{
		// 首帧给出当前累计平均，避免输出长期保持全 0 的误解。
		sdHeldOutput_ = std::complex<double>(bi * vref_, bq * vref_);
	}

	return sdHeldOutput_;
}


// -----------------------------------------------------------------------------
// SNR / 工具函数
// -----------------------------------------------------------------------------

double AtoD::target_snr_db_() const
{
	double idealSNR = 6.02 * static_cast<double>(nbits_) + 1.76;

	if (DistortionModel == ENOB_value)
	{
		double snr = 6.02 * ENOB + 1.76;

		// 内置会根据 NBits、Level_dBFS 等限制上限。
		// 这里做一个简单上限限制，避免明显超过量化极限。
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


double AtoD::full_scale_peak_() const
{
	// 文档中 VRef 是正弦输入的 peak amplitude，不是 RMS。
	// Level_dBFS=0 时，peak 取 VRef。
	return std::max(vref_ * db_to_amp_(Level_dBFS), kTiny);
}


double AtoD::uniform_()
{
	rngState_ ^= (rngState_ << 13);
	rngState_ ^= (rngState_ >> 17);
	rngState_ ^= (rngState_ << 5);

	return (static_cast<double>(rngState_) + 1.0) / 4294967297.0;
}


double AtoD::gaussian_()
{
	double u1 = std::max(uniform_(), 1e-12);
	double u2 = std::max(uniform_(), 1e-12);

	double r = std::sqrt(-2.0 * std::log(u1));
	double th = 2.0 * kPi * u2;

	return r * std::cos(th);
}


double AtoD::db_to_amp_(double dB)
{
	return std::pow(10.0, dB / 20.0);
}


double AtoD::clip_(double x, double lo, double hi)
{
	if (x < lo) return lo;
	if (x > hi) return hi;
	return x;
}


int AtoD::clamp_int_(int v, int lo, int hi)
{
	if (v < lo) return lo;
	if (v > hi) return hi;
	return v;
}
