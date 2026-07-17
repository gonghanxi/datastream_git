#pragma once

#include "ModelBuilder.h"
#include "TimedDFModel.h"
#include "EnvelopeSignal.h"

#include <complex>
#include <vector>
#include <deque>
#include <cstdint>

class RADAR_Tx_4x4 : public SystemVueModelBuilder::TimedDFModel
{
public:
	DECLARE_MODEL_INTERFACE(RADAR_Tx_4x4);

	RADAR_Tx_4x4();

	virtual bool Setup();
	virtual bool Run();
	virtual ERESULT PropagateCharacterizationFrequency();

	enum SelectedGCType
	{
		none = 0,
		TOI = 1,
		dBc1 = 2,
		TOI_dBc1 = 3,
		PSat_GCSat_TOI = 4,
		PSat_GCSat_dBc1 = 5,
		PSat_GCSat_TOI_dBc1 = 6,
		RappNonlinearity = 7,
		Gain_compression_vs_input_power = 8,
		AM_AM_and_AMPM_vs_input_power = 9
	};

	typedef std::complex<double> Cx;

	// ============================================================
	// ============================================================
	SystemVueModelBuilder::CircularBufferBusT<SystemVueModelBuilder::DComplexCircularBuffer> BB_Signal;
	SystemVueModelBuilder::EnvelopeCircularBufferBus RF_Signal;

	// ============================================================
	// ============================================================
	double TStep;
	double RF_Freq;
	std::complex<double> RF_Gain;

	double IF_Freq;
	std::complex<double> IF_Gain;

	double IF_SamplingRate;
	double BandWidth;
	double In_CenterFreq;

	int    BB_UpSamplingRatio;
	double RC_ExcessBW;
	double PhaseImbalance;

	int    DAC_NBits;
	int    DAC_UpSamplingRatio;

	int    NumTxAnt;
	double ChannelDelay;

	double NoiseFigure_RF_Gain;
	double NoiseFigure_IF_Gain;
	double NoiseFigure_Mixer;

	// ============================================================
	// ============================================================
	SelectedGCType GCType_RF_Gain;
	double TOIout_RF_Gain;
	double dBc1out_RF_Gain;
	double PSat_RF_Gain;
	double GCSat_RF_Gain;
	int    RappS_RF_Gain;

	double* GComp_RF_Gain;
	int     GComp_RF_Gain_Size;

	// ============================================================
	// ============================================================
	SelectedGCType GCType_IF_Gain;
	double TOIout_IF_Gain;
	double dBc1out_IF_Gain;
	double PSat_IF_Gain;
	double GCSat_IF_Gain;
	int    RappS_IF_Gain;

	double* GComp_IF_Gain;
	int     GComp_IF_Gain_Size;

private:
	struct GCompTable
	{
		bool valid;
		std::vector<double> pinDbm;
		std::vector<double> gainChangeDb;
		std::vector<double> phaseChangeDeg;

		GCompTable()
			: valid(false)
		{
		}
	};

	struct BiquadState
	{
		double b0;
		double b1;
		double b2;
		double a1;
		double a2;

		Cx x1;
		Cx x2;
		Cx y1;
		Cx y2;

		BiquadState();
		void reset();
	};

	struct ChannelState
	{
		std::deque<Cx> ducFirState;
		Cx ducHold;

		BiquadState ifBpfSec1;
		BiquadState ifBpfSec2;
		BiquadState rfBpfSec1;
		BiquadState rfBpfSec2;

		std::deque<Cx> delayLine;

		uint32_t seedRF;
		uint32_t seedIF;
		uint32_t seedMixer;

		unsigned long long outputCount;

		double lastRfAbs;
		double edgeRippleState;

		double riseEdgeState;
		double fallEdgeState;

		bool inPulse;
		unsigned long long pulseSampleIndex;

		ChannelState();
		void resetRuntime();
	};

private:
	double sampleRateHz_;
	double timeStepSec_;
	double outputSampleRateHz_;
	double outputTimeStepSec_;

	int bbUp_;
	int dacUp_;
	int outRate_;
	size_t activeChannels_;
	int channelDelaySamples_;

	bool noisePrepared_;
	double noiseSigmaRF_;
	double noiseSigmaIF_;
	double noiseSigmaMixer_;

	GCompTable rfTable_;
	GCompTable ifTable_;

	std::vector<double> ducFir_;

	bool ifBpfEnabled_;
	bool rfBpfEnabled_;
	BiquadState ifBpfProtoSec1_;
	BiquadState ifBpfProtoSec2_;
	BiquadState rfBpfProtoSec1_;
	BiquadState rfBpfProtoSec2_;

	std::vector<ChannelState> ch_;

private:
	bool prepareTables();
	bool prepareNoise();

	bool parseGCompArray(const double* data,
		int size,
		GCompTable& table) const;

	void applyRates_();
	void applyOutputTiming_();
	void resetStates_();
	void resizeChannels_();
	int computeChannelDelaySamples_() const;

	void buildRaisedCosineFir_();
	Cx runDucInterpolationFir_(const Cx& x, ChannelState& st);

	void configureIfBpf_();
	void configureRfBpf_();
	Cx runBiquad(const Cx& x, BiquadState& s);
	Cx runIfBpf_(const Cx& x, ChannelState& st);
	Cx runRfBpf_(const Cx& x, ChannelState& st);
	Cx applyChannelDelay_(const Cx& x, ChannelState& st);

	Cx applyInputCenterFrequency_(const Cx& x,
		double timeNow) const;

	Cx applyDUCToIFEnvelope_(const Cx& x,
		double timeNow) const;

	Cx applyFcChangeImage_(const Cx& idealEnvelope,
		double timeNow,
		ChannelState& st);

	Cx applyFinalComplexPhaseCorrection_(const Cx& x,
		double timeNow) const;

	Cx applyMixerToRFEnvelope_(const Cx& x,
		double timeNow) const;

	double applyDAC_(double x) const;

	Cx addNoise(const Cx& x,
		double sigma,
		uint32_t& seed);

	Cx applyStage(const Cx& x,
		const Cx& gain,
		SelectedGCType gcType,
		double toiOut,
		double dbc1Out,
		double psat,
		double gcSat,
		int rappS,
		const GCompTable& table) const;

	double applyCompressionMagnitude(double ain,
		double gainAbs,
		SelectedGCType gcType,
		double toiOut,
		double dbc1Out,
		double psat,
		double gcSat,
		int rappS,
		const GCompTable& table) const;

	Cx applyTableCompressionComplex(const Cx& yLinear,
		double ain,
		double gainAbs,
		SelectedGCType gcType,
		const GCompTable& table) const;

	bool lookupTable(double pinDbm,
		const GCompTable& table,
		double& gainChangeDb,
		double& phaseChangeDeg) const;

	double applyTOI(double ain,
		double gainAbs,
		double toiOut) const;

	double applydBc1(double ain,
		double gainAbs,
		double dbc1Out) const;

	double applyTOIdBc1(double ain,
		double gainAbs,
		double toiOut,
		double dbc1Out) const;

	double applyPSat(double ain,
		double gainAbs,
		double psat,
		double gcSat) const;

	double applyRapp(double ain,
		double gainAbs,
		double psat,
		int rappS) const;

	double applyTableCompressionMagnitude(double ain,
		double gainAbs,
		const GCompTable& table) const;

	double randUniform_(uint32_t& seed) const;
	double randn_(uint32_t& seed) const;

	static double raisedCosineImpulse_(double t, double alpha);
	static double sinc_(double x);

	static double dbToLinVoltage(double db);
	static double linToDbVoltage(double lin);
	static double wattToDbm(double w);
	static double dbmToWatt(double dbm);
	static double wattToPeakVoltage(double w, double r);
	static double peakVoltageToWatt(double v, double r);
	static double peakVoltageToDbm(double v, double r);
	static double dbmToPeakVoltage(double dbm, double r);
	static double deg2rad(double x);
	static double clamp(double x, double lo, double hi);
};
