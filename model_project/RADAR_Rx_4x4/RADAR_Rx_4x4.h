#pragma once

#include "ModelBuilder.h"
#include "TimedDFModel.h"
#include "EnvelopeSignal.h"

#include <complex>
#include <vector>
#include <deque>
#include <cstdint>

class RADAR_Rx_4x4 : public SystemVueModelBuilder::TimedDFModel
{
public:
	DECLARE_MODEL_INTERFACE(RADAR_Rx_4x4);

	RADAR_Rx_4x4();

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
	// Ports
	// Port 1 : RF_Signal, multiple envelope
	// Port 2 : BB_Signal, multiple complex
	// ============================================================
	SystemVueModelBuilder::EnvelopeCircularBufferBus RF_Signal;
	SystemVueModelBuilder::CircularBufferBusT<SystemVueModelBuilder::DComplexCircularBuffer> BB_Signal;

	// ============================================================
	// Basic receiver parameters
	// ============================================================
	double TStep;
	double RF_Freq;
	std::complex<double> RF_Gain;

	double IF_Freq;
	std::complex<double> IF_Gain;

	double IF_SamplingRate;
	double BandWidth;

	int    ADC_NBits;
	double PhaseImbalance;
	int    BB_DownSamplingRatio;
	double RC_ExcessBW;
	double Out_CenterFreq;

	double NoiseFigure_RFGain;
	double NoiseFigure_IFGain;
	double NoiseFigure_Mixer;

	// ============================================================
	// RF gain compression parameters
	// ============================================================
	SelectedGCType GCType_RFGain;
	double TOIout_RFGain;
	double dBc1out_RFGain;
	double PSat_RFGain;
	double GCSat_RFGain;

	double* GComp_RFGain;
	int     GComp_RFGain_Size;

	// ============================================================
	// IF gain compression parameters
	// ============================================================
	SelectedGCType GCType_IFGain;
	double TOIout_IFGain;
	double dBc1out_IFGain;
	double PSat_IFGain;
	double GCSat_IFGain;

	double* GComp_IFGain;
	int     GComp_IFGain_Size;

	// ============================================================
	// 4x4 / multi-channel parameters
	// ============================================================
	int    NumRxAnt;
	double ChannelDelay;

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
		double inputFcHz;

		// Each channel owns its own IF BPF states.
		BiquadState bpfSec1;
		BiquadState bpfSec2;
		BiquadState bpfSec3;
		BiquadState bpfSec4;

		// Delay line is placed after IF gain and before DDC.
		std::deque<Cx> delayLine;

		// Deterministic per-channel noise states.
		uint32_t seedRF;
		uint32_t seedIF;
		uint32_t seedMixer;

		long outputCount;

		ChannelState();
		void reset();
	};

private:
	size_t inBusSize_;
	size_t outBusSize_;
	size_t activeChannels_;

	double sampleRateHz_;
	double timeStepSec_;
	int    decim_;
	int    delaySamples_;

	bool   noisePrepared_;
	double noiseSigmaRF_;
	double noiseSigmaIF_;
	double noiseSigmaMixer_;

	bool bpfEnabled_;
	bool useLowFreqStartupCorrection_;

	GCompTable rfTable_;
	GCompTable ifTable_;

	std::vector<ChannelState> ch_;

private:
	bool prepareTables();
	bool prepareNoise();

	bool parseGCompArray(const double* data,
		int size,
		GCompTable& table) const;

	void configureBpfFilter();
	void resetChannelStates();

	void applyInputRates_();
	void applyOutputTiming_();

	int computeDelaySamples_() const;
	bool isLowFreqStartupCorrectionCase_() const;
	Cx applyLowFreqStartupCorrection_(const Cx& x, long outputCount) const;
	Cx applyLowFreqSteadyPhaseCorrection_(const Cx& x, long outputCount) const;

	Cx runBpfFilter(const Cx& x, ChannelState& st);
	Cx runBiquad(const Cx& x, BiquadState& s);

	Cx envelopeToComplex(
		const SystemVueModelBuilder::EnvelopeSignal& x,
		double fcHz) const;

	Cx addNoise(
		const Cx& x,
		double sigma,
		uint32_t& seed);

	Cx applyMixerToIF(
		const Cx& x,
		double inputFcHz,
		double timeNow) const;

	Cx applyChannelDelay(
		const Cx& x,
		ChannelState& st) const;

	Cx applyDDCToBaseband(
		const Cx& x,
		double timeNow) const;

	Cx applyPhaseImbalance(
		const Cx& x) const;

	Cx applyADC(
		const Cx& x) const;

	Cx applyStage(
		const Cx& x,
		const Cx& gain,
		SelectedGCType gcType,
		double toiOut,
		double dbc1Out,
		double psat,
		double gcSat,
		const GCompTable& table) const;

	double applyCompressionMagnitude(
		double ain,
		double gainAbs,
		SelectedGCType gcType,
		double toiOut,
		double dbc1Out,
		double psat,
		double gcSat,
		const GCompTable& table) const;

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
		double psat) const;

	double applyTableCompression(double ain,
		double gainAbs,
		const GCompTable& table) const;

	double randUniform_(uint32_t& seed) const;
	double randn_(uint32_t& seed) const;

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
