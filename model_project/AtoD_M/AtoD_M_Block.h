#ifndef ATOD_M_BLOCK_H
#define ATOD_M_BLOCK_H

#include "Block.h"
#include "AtoD_M.h"
#include <queue>
#include <vector>
#include <complex>
#include <cstdlib>
#include <memory>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API AtoD_M_Block : public SystemVueModelBuilder::Block
{
public:
	AtoD_M_Block(const std::string& name);
	~AtoD_M_Block() = default;

	bool Setup() override;
	bool Run() override;
	bool Initialize() override;

private:
	// ==== TimeDrivenRun 缓冲/队列结构 ====
	struct InputSnapshot
	{
		EnvelopeMatrix matrix;
	};

	struct OutputFrame
	{
		EnvelopeMatrix A_out;
		IntMatrix       D_I;
		IntMatrix       D_Q;
	};

	// ==== 内部状态结构 ====
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
			: hasClockState(false), lastClockValue(0.0), heldSample(0.0, 0.0)
			, hasPendingClockSample(false), pendingClockSample(0.0, 0.0)
			, hasRawInputState(false), prevRawInputTime(0.0), prevRawInput(0.0, 0.0)
			, hasNextClockCrossing(false), nextClockCrossingTime(0.0) {}
	};

	struct DistortionState
	{
		bool hasLastInput;
		double lastInputTime;
		std::complex<double> lastInput;

		DistortionState()
			: hasLastInput(false), lastInputTime(0.0), lastInput(0.0, 0.0) {}
	};

	// ==== 参数/枚举解析 ====
	void SetDefaultParamters();
	void SetParameters();
	bool parseArrayString(const std::string& arrayStr, std::vector<double>& outArray);

	std::unique_ptr<AtoD_M> m_atod_m;

	AtoD_M::OutputDigitalFormatEnum ConvertStringToOutputDigitalFormatEnum(const std::string& value);
	AtoD_M::DistortionModelEnum     ConvertStringToDistortionModelEnum(const std::string& value);
	AtoD_M::EnableJitterEnum        ConvertStringToEnableJitterEnum(const std::string& value);
	AtoD_M::PN_TypeEnum             ConvertStringToPN_TypeEnum(const std::string& value);
	AtoD_M::FFT_SizeEnum            ConvertStringToFFT_SizeEnum(const std::string& value);
	AtoD_M::SNR_ModelEnum           ConvertStringToSNR_ModelEnum(const std::string& value);
	AtoD_M::ConversionTypeEnum      ConvertStringToConversionTypeEnum(const std::string& value);
	AtoD_M::AntiAliasingFilterEnum  ConvertStringToAntiAliasingFilterEnum(const std::string& value);

	// ==== 双模式 Run ====
	bool DataStreamRun();
	bool TimeDrivenRun();

	// ==== 算法内联方法 ====
	void clamp_params_();
	void build_transfer_table_();
	void reset_states_();
	void ensure_state_count_(std::size_t n);

	std::complex<double> read_matrix_sample_(const EnvelopeMatrix& m, std::size_t elem);
	std::vector<std::complex<double>> get_downsampled_matrix_(const std::vector<EnvelopeMatrix>& inputVec, std::size_t elemCount);
	std::complex<double> get_clocked_input_(const std::complex<double>& x, double t, ClockState& st);

	double first_positive_crossing_at_or_after_(double t) const;
	std::complex<double> interp_(const std::complex<double>& x0, double t0,
		const std::complex<double>& x1, double t1, double ts) const;

	std::complex<double> apply_distortion_(const std::complex<double>& x, double t, DistortionState& st);
	std::complex<double> apply_jitter_(const std::complex<double>& x, double t, DistortionState& st);
	std::complex<double> apply_phase_noise_(const std::complex<double>& x);
	std::complex<double> apply_harmonics_(const std::complex<double>& x,
		double h2_dBc, double h3_dBc, double h4_dBc, double h5_dBc) const;
	std::complex<double> apply_sinad_sfdr_(const std::complex<double>& x) const;
	std::complex<double> apply_noise_(const std::complex<double>& x, double snr_dB);

	double apply_harmonics_real_(double x,
		double h2_dBc, double h3_dBc, double h4_dBc, double h5_dBc) const;

	QuantResult quantize_(double x) const;

	double target_snr_db_() const;
	double full_scale_peak_() const;

	double uniform_();
	double gaussian_();

	static double db_to_amp_(double dB);
	static double clip_(double x, double lo, double hi);
	static int    clamp_int_(int v, int lo, int hi);

	static const double kPi;
	static const double kTiny;

	// ==== TimeDrivenRun 缓冲/队列 ====
	std::vector<InputSnapshot> m_inputBuffer;
	std::queue<OutputFrame>    m_outputQueue;

	// ==== 参数 ====
	int    m_NBits;
	double m_VRef;

	AtoD_M::OutputDigitalFormatEnum m_OutputDigitalFormat;
	AtoD_M::DistortionModelEnum     m_DistortionModel;

	AtoD_M::EnableJitterEnum m_EnableJitter;
	double m_RJrms;

	std::vector<double> m_PhaseNoiseVector;
	AtoD_M::PN_TypeEnum m_PN_Type;

	double m_INL;
	double m_DNL;

	double m_ENOB;
	double m_SNR_dB;
	double m_H2_dBc;
	double m_H3_dBc;
	double m_H4_dBc;
	double m_H5_dBc;

	double m_SINAD_dB;
	double m_SFDR_dBc;
	AtoD_M::FFT_SizeEnum m_FFT_Size;

	AtoD_M::SNR_ModelEnum m_SNR_Model;
	double m_ThermalNoise_SNR_dBFS;
	double m_CenterFreq;
	double m_Level_dBFS;

	AtoD_M::ConversionTypeEnum     m_ConversionType;
	double m_Clock;
	double m_Phase;

	int m_DownsampleFactor;
	int m_DownsamplePhase;
	AtoD_M::AntiAliasingFilterEnum m_AntiAliasingFilter;
	double m_ExcessBW;

	// ==== 内部派生状态 ====
	int    m_nbits;
	int    m_codeCount;
	int    m_midCode;
	double m_vref;
	double m_lsb;
	unsigned long long m_sampleIndex;
	unsigned int       m_rngState;

	std::vector<double> m_thresholds;
	std::vector<double> m_levels;

	std::vector<ClockState>      m_clockStates;
	std::vector<DistortionState> m_distortionStates;

	SimuParameter m_simulator_param;
    int system_rate = 1;
};

RegAlgo(AtoD_M_Block);
#endif // ATOD_M_BLOCK_H
