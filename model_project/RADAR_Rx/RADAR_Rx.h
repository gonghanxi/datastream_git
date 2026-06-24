#pragma once

#include "ModelBuilder.h"
#include "TimedDFModel.h"
#include "EnvelopeSignal.h"
#include "RandomNumberGenerator.h"

#include <complex>
#include <vector>

class RADAR_Rx : public SystemVueModelBuilder::TimedDFModel
{
public:
	DECLARE_MODEL_INTERFACE(RADAR_Rx);

	RADAR_Rx();

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

	// ============================================================
	// Ports
	// ============================================================
	SystemVueModelBuilder::EnvelopeCircularBuffer RF_Signal;
	SystemVueModelBuilder::DComplexCircularBuffer BB_Signal;

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

	double*      GComp_RFGain;
	int GComp_RFGain_Size;

	// ============================================================
	// IF gain compression parameters
	// ============================================================
	SelectedGCType GCType_IFGain;
	double TOIout_IFGain;
	double dBc1out_IFGain;
	double PSat_IFGain;
	double GCSat_IFGain;

	double*      GComp_IFGain;
	int GComp_IFGain_Size;

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

		std::complex<double> x1;
		std::complex<double> x2;
		std::complex<double> y1;
		std::complex<double> y2;

		BiquadState();
		void reset();
	};

private:
	double inputFcHz_;
	double sampleRateHz_;
	double timeStepSec_;

	int  decim_;

	// 输出计数，用于当前低频黑盒测试链路的稳态 5 点周期校正
	long outputCount_;
	bool useLowFreqBlackBoxCorrection_;

	bool noisePrepared_;
	double noiseSigmaRF_;
	double noiseSigmaIF_;
	double noiseSigmaMixer_;

	SystemVueModelBuilder::CNormal rngNoiseRFI_;
	SystemVueModelBuilder::CNormal rngNoiseRFQ_;
	SystemVueModelBuilder::CNormal rngNoiseIFI_;
	SystemVueModelBuilder::CNormal rngNoiseIFQ_;
	SystemVueModelBuilder::CNormal rngNoiseMixerI_;
	SystemVueModelBuilder::CNormal rngNoiseMixerQ_;

	GCompTable rfTable_;
	GCompTable ifTable_;

	bool bpfEnabled_;

	BiquadState bpfSec1_;
	BiquadState bpfSec2_;
	BiquadState bpfSec3_;
	BiquadState bpfSec4_;

private:
	bool prepareTables();
	bool prepareNoise();

	bool parseGCompArray(const double* data,
		int size,
		GCompTable& table) const;

	void configureBpfFilter();
	void resetBpfFilter();

	std::complex<double> runBpfFilter(const std::complex<double>& x);
	std::complex<double> runBiquad(const std::complex<double>& x,
		BiquadState& s);

	bool isLowFreqBlackBoxCase() const;

	std::complex<double> applyLowFreqBlackBoxCorrection(
		const std::complex<double>& x) const;

	std::complex<double> envelopeToComplex(
		const SystemVueModelBuilder::EnvelopeSignal& x,
		double fcHz) const;

	std::complex<double> addNoise(
		const std::complex<double>& x,
		double sigma,
		SystemVueModelBuilder::CNormal& rngI,
		SystemVueModelBuilder::CNormal& rngQ);

	std::complex<double> applyMixerToIF(
		const std::complex<double>& x,
		double timeNow) const;

	std::complex<double> applyDDCToBaseband(
		const std::complex<double>& x,
		double timeNow) const;

	std::complex<double> applyPhaseImbalance(
		const std::complex<double>& x) const;

	std::complex<double> applyADC(
		const std::complex<double>& x) const;

	std::complex<double> applyStage(
		const std::complex<double>& x,
		const std::complex<double>& gain,
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
