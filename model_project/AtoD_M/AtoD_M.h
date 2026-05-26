#pragma once

#include "ModelBuilder.h"
#include "TimedDFModel.h"
#include "EnvelopeSignal.h"
#include "CircularBuffer.h"
#include "Matrix.h"
#include "MatrixCircularBuffer.h"

#include <cmath>
#include <complex>
#include <vector>
#include <cstddef>

// Matrix version of AtoD.
// 与 AtoD 的核心计算保持一致，只是端口改为 matrix：
//   A_in  : envelope matrix
//   A_out : envelope matrix
//   D_I   : integer matrix
//   D_Q   : integer matrix
//
// 说明：每个矩阵元素按一个独立的 AtoD 通道处理，共用同一组模型参数和同一个内部时钟。
// 随机/统计类失真模型仍然只能趋势近似，不能逐点复现内置随机序列。
class SYSTEMVUEMODELBUILDER_API AtoD_M : public SystemVueModelBuilder::TimedDFModel
{
public:
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

	DECLARE_MODEL_INTERFACE(AtoD_M);

	AtoD_M();

	ERESULT PropagateCharacterizationFrequency();
	bool Setup() override;
	bool Run() override;

	// --------- 端口 ---------
	// Port 1：A_in，envelope matrix
	SystemVueModelBuilder::EnvelopeMatrixCircularBuffer A_in;

	// Port 2：A_out，envelope matrix
	SystemVueModelBuilder::EnvelopeMatrixCircularBuffer A_out;

	// Port 3：D_I，integer matrix
	SystemVueModelBuilder::IntMatrixCircularBuffer D_I;

	// Port 4：D_Q，integer matrix
	SystemVueModelBuilder::IntMatrixCircularBuffer D_Q;

	// --------- 参数 ---------
    std::vector<SystemVueModelBuilder::EnvelopeMatrix> A_Input;

	int    NBits;
	double VRef;

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

private:
	struct QuantResult
	{
		int    codeOffset;
		int    codeDigital;
		double analog;
	};

	struct ClockState
	{
		bool hasClockState;
		double lastClockValue;
		std::complex<double> heldSample;

		bool hasPendingClockSample;
		std::complex<double> pendingClockSample;

		bool hasRawInputState;
		double prevRawInputTime;
		std::complex<double> prevRawInput;

		bool hasNextClockCrossing;
		double nextClockCrossingTime;

		ClockState()
			: hasClockState(false)
			, lastClockValue(0.0)
			, heldSample(0.0, 0.0)
			, hasPendingClockSample(false)
			, pendingClockSample(0.0, 0.0)
			, hasRawInputState(false)
			, prevRawInputTime(0.0)
			, prevRawInput(0.0, 0.0)
			, hasNextClockCrossing(false)
			, nextClockCrossingTime(0.0)
		{
		}
	};

	struct DistortionState
	{
		bool hasLastInput;
		double lastInputTime;
		std::complex<double> lastInput;

		DistortionState()
			: hasLastInput(false)
			, lastInputTime(0.0)
			, lastInput(0.0, 0.0)
		{
		}
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
	unsigned int rngState_;

	std::vector<double> thresholds_;
	std::vector<double> levels_;

	std::vector<ClockState> clockStates_;
	std::vector<DistortionState> distortionStates_;

private:
	void clamp_params_();
	void build_transfer_table_();

	void reset_states_();
	void ensure_state_count_(std::size_t n);

	std::complex<double> read_matrix_sample_(int idx, std::size_t elem);
	std::vector<std::complex<double> > get_downsampled_matrix_(std::size_t elemCount);
	std::complex<double> get_clocked_input_(const std::complex<double>& x, double t, ClockState& st);

	double first_positive_crossing_at_or_after_(double t) const;
	std::complex<double> interp_(const std::complex<double>& x0,
		double t0,
		const std::complex<double>& x1,
		double t1,
		double ts) const;

	std::complex<double> apply_distortion_(const std::complex<double>& x, double t, DistortionState& st);
	std::complex<double> apply_jitter_(const std::complex<double>& x, double t, DistortionState& st);
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

	QuantResult quantize_(double x) const;

	double target_snr_db_() const;
	double full_scale_peak_() const;

	double uniform_();
	double gaussian_();

	static double db_to_amp_(double dB);
	static double clip_(double x, double lo, double hi);
	static int    clamp_int_(int v, int lo, int hi);
};
