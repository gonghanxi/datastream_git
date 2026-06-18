#pragma once

#include "ModelBuilder.h"
#include "TimedDFModel.h"
#include "EnvelopeSignal.h"
#include "CircularBuffer.h"

#include <cmath>
#include <complex>
#include <vector>

// 与 SystemVue 2020 内置 AtoD 行为尽量接近的自设实现。
// 重点对齐：
//   1. envelope 输入/输出；
//   2. I/Q 双路独立量化；
//   3. Offset binary / Twos-complement 数字码输出；
//   4. A_out 输出码中心电平；
//   5. Clocked 采样保持与 Downsampled 抽取；
//   6. SNR and Harmonics / SINAD and SFDR 的频谱近似。
//
// V2 架构升级：
//   ADCType = Current AtoD 时完全沿用原有最接近内置 AtoD 的逻辑；
//   ADCType = Flash / Pipeline / SigmaDelta 时仅增加基础架构转换功能，
//   不改变 Current AtoD 分支中已有的采样、失真、量化和时序实现。
//
// 注意：Jitter、Phase Noise、INL/DNL、ENOB 等随机/统计模型无法逐点复现内置随机序列。
class AtoD : public SystemVueModelBuilder::TimedDFModel
{
public:
	enum ADCTypeEnum
	{
		Current_AtoD = 0,
		Flash_ADC = 1,
		Pipeline_ADC = 2,
		SigmaDelta_ADC = 3
	};

	enum OutputDigitalFormatEnum
	{
		Offset_binary = 0,
		Twos_complement = 1
	};

	enum DistortionModelEnum
	{
		Distortion_None = 0,
		Jitter_INL_DNL = 1,
		ENOB_value = 2,
		SNR_and_Harmonics = 3,
		SINAD_and_SFDR = 4
	};

	enum EnableJitterEnum
	{
		Jitter_No = 0,
		Time_Domain = 1,
		Frequency_Domain = 2
	};

	enum PN_TypeEnum
	{
		Random_PN = 0,
		Fixed_freq_offset = 1,
		Fixed_freq_offset_and_amplitude = 2
	};

	enum FFT_SizeEnum
	{
		FFT_2_12 = 0,
		FFT_2_13 = 1,
		FFT_2_14 = 2,
		FFT_2_15 = 3,
		FFT_2_16 = 4
	};

	enum SNR_ModelEnum
	{
		Quantization_and_Jitter = 0,
		Quantization_and_INL_DNL = 1,
		Quantization_and_Jitter_or_INL_DNL = 2,
		Quantization_Jitter_and_Thermal_Noise = 3
	};

	enum ConversionTypeEnum
	{
		Clocked = 0,
		Downsampled = 1
	};

	enum AntiAliasingFilterEnum
	{
		AA_OFF = 0,
		AA_ON = 1
	};

	DECLARE_MODEL_INTERFACE(AtoD);

	AtoD();

	ERESULT PropagateCharacterizationFrequency();
	bool Setup() override;
	bool Run() override;

	// --------- 端口 ---------
	// Port 1：A_in，envelope
	SystemVueModelBuilder::EnvelopeCircularBuffer A_in;

	// Port 2：A_out，envelope
	SystemVueModelBuilder::EnvelopeCircularBuffer A_out;

	// Port 3：D_I，int
	SystemVueModelBuilder::CircularBuffer<int> D_I;

	// Port 4：D_Q，int
	SystemVueModelBuilder::CircularBuffer<int> D_Q;

	// --------- 参数 ---------
	int    NBits;
	double VRef;

	// 新增架构选择。默认 Current_AtoD，不污染原有最接近内置 AtoD 的逻辑。
	ADCTypeEnum ADCType;

	OutputDigitalFormatEnum OutputDigitalFormat;
	DistortionModelEnum     DistortionModel;

	EnableJitterEnum EnableJitter;
	double RJrms;

	double* PhaseNoiseData;
	int     PhaseNoiseDataSize;
	PN_TypeEnum PN_Type;

	double INL;
	double DNL;

	double ENOB;
	double SNR_dB;
	double H2_dBc;
	double H3_dBc;
	double H4_dBc;
	double H5_dBc;

	double SINAD_dB;
	double SFDR_dBc;
	FFT_SizeEnum FFT_Size;

	SNR_ModelEnum SNR_Model;
	double ThermalNoise_SNR_dBFS;
	double CenterFreq;
	double Level_dBFS;

	ConversionTypeEnum ConversionType;
	double Clock;
	double Phase;

	int DownsampleFactor;
	int DownsamplePhase;
	AntiAliasingFilterEnum AntiAliasingFilter;
	double ExcessBW;

	// Pipeline ADC 基础功能参数：默认 0 延迟，避免影响基础转换验证。
	int PipelineStageBits;
	int PipelineLatency;

	// Sigma-Delta ADC 基础功能参数：一阶 1-bit 调制器 + OSR 移动平均抽取近似。
	int SigmaDeltaOrder;
	int SigmaDeltaOSR;

private:
	struct QuantResult
	{
		int    codeOffset;
		int    codeDigital;
		double analog;
	};

	template<typename Buffer>
	static auto try_set_rate_(Buffer& b, int r, int) -> decltype(b.SetRate(static_cast<unsigned>(r)), void())
	{
		if (r < 1) r = 1;
		b.SetRate(static_cast<unsigned>(r));
	}

	template<typename Buffer>
	static void try_set_rate_(Buffer&, int, ...)
	{
		// 某些 Buffer 类型没有 SetRate 接口时保持默认速率。
	}

private:
	static const double kPi;
	static const double kTiny;

	int nbits_;
	int codeCount_;
	int midCode_;
	double vref_;
	double lsb_;

	unsigned long long sampleIndex_;

	// Clocked 模式状态：内置 AtoD 更接近“余弦时钟正向过零 + 采样保持”。
	bool hasClockState_;
	double lastClockValue_;
	std::complex<double> heldSample_;

	// Clocked 模式：采样沿命中新值延后一帧输出，用于对齐内置 AtoD 的 sample/hold 输出时序。
	bool hasPendingClockSample_;
	std::complex<double> pendingClockSample_;

	bool hasRawInputState_;
	double prevRawInputTime_;
	std::complex<double> prevRawInput_;

	bool hasNextClockCrossing_;
	double nextClockCrossingTime_;

	// 失真模型中的斜率/抖动近似状态。
	bool hasLastInput_;
	double lastInputTime_;
	std::complex<double> lastInput_;

	unsigned int rngState_;

	std::vector<double> thresholds_;
	std::vector<double> levels_;

	// Pipeline ADC 基础流水延迟状态。
	std::vector<std::complex<double> > pipelineFifo_;

	// Sigma-Delta ADC 基础一阶调制器状态。
	double sdIIntegrator_;
	double sdQIntegrator_;
	double sdIFeedback_;
	double sdQFeedback_;
	double sdIAccum_;
	double sdQAccum_;
	int    sdAccumCount_;
	std::complex<double> sdHeldOutput_;

private:
	void clamp_params_();
	void build_transfer_table_();

	std::complex<double> read_input_sample_(int idx);
	std::complex<double> get_downsampled_input_();
	std::complex<double> get_clocked_input_(const std::complex<double>& x, double t);

	double first_positive_crossing_at_or_after_(double t) const;
	std::complex<double> interp_(const std::complex<double>& x0,
		double t0,
		const std::complex<double>& x1,
		double t1,
		double ts) const;

	std::complex<double> apply_distortion_(const std::complex<double>& x, double t);
	std::complex<double> apply_jitter_(const std::complex<double>& x, double t);
	std::complex<double> apply_phase_noise_(const std::complex<double>& x);
	std::complex<double> apply_harmonics_(const std::complex<double>& x,
		double h2_dBc,
		double h3_dBc,
		double h4_dBc,
		double h5_dBc) const;
	std::complex<double> apply_sinad_sfdr_(const std::complex<double>& x) const;
	std::complex<double> apply_noise_(const std::complex<double>& x, double snr_dB);

	double apply_harmonics_real_(double x,
		double h2_dBc,
		double h3_dBc,
		double h4_dBc,
		double h5_dBc) const;

	// Current_AtoD / Pipeline / SigmaDelta 最终使用该量化核心；Flash 使用比较器计数核心。
	QuantResult quantize_(double x) const;
	QuantResult quantize_flash_(double x) const;

	std::complex<double> process_pipeline_(const std::complex<double>& x);
	std::complex<double> process_sigma_delta_(const std::complex<double>& x);

	double target_snr_db_() const;
	double full_scale_peak_() const;

	double uniform_();
	double gaussian_();

	static double db_to_amp_(double dB);
	static double clip_(double x, double lo, double hi);
	static int    clamp_int_(int v, int lo, int hi);
};
