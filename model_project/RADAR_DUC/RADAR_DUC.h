#pragma once

#include "ModelBuilder.h"
#include "TimedDFModel.h"
#include "EnvelopeSignal.h"

#include <complex>
#include <vector>
#include <deque>

class RADAR_DUC : public SystemVueModelBuilder::TimedDFModel
{
public:
	DECLARE_MODEL_INTERFACE(RADAR_DUC);

	RADAR_DUC();

	virtual bool Setup();
	virtual bool Run();
	virtual ERESULT PropagateCharacterizationFrequency();

	typedef std::complex<double> Cx;

	// ============================================================
	// 端口定义
	// 端口 1：BB_Signal，complex 基带复信号输入
	// 端口 2：IF_Signal，envelope 中频包络输出
	// ============================================================
	SystemVueModelBuilder::DComplexCircularBuffer BB_Signal;
	SystemVueModelBuilder::EnvelopeCircularBuffer IF_Signal;

	// ============================================================
	// RADAR_DUC 帮助文档参数
	// ============================================================
	double IF_Freq;
	double IF_SamplingRate;
	double BandWidth;
	double In_CenterFreq;
	int    BB_UpSamplingRatio;
	double RC_ExcessBW;
	double PhaseImbalance;
	int    DAC_NBits;

private:
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

private:
	double outputSampleRateHz_;
	double outputTimeStepSec_;
	int    upRate_;
	int    outRate_;

	std::vector<double> ducFir_;
	std::deque<Cx>      ducFirState_;

	bool        ifBpfEnabled_;
	BiquadState ifBpfSec1_;
	BiquadState ifBpfSec2_;

private:
	void applyRates_();
	void applyOutputTiming_();
	void resetStates_();

	void buildRaisedCosineFir_();
	Cx   runDucInterpolationFir_(const Cx& x);

	void configureIfBpf_();
	Cx   runBiquad_(const Cx& x, BiquadState& s);
	Cx   runIfBpf_(const Cx& x);

	Cx applyInputCenterFrequency_(const Cx& x, double timeNow) const;
	Cx applyDUCToIFEnvelope_(const Cx& x, double timeNow) const;
	Cx applyFcChangeImage_(const Cx& idealEnvelope, double timeNow) const;
	Cx applyFinalComplexConvention_(const Cx& x, double timeNow) const;

	double applyDAC_(double x) const;

	static double raisedCosineImpulse_(double t, double alpha);
	static double sinc_(double x);
	static double deg2rad(double x);
	static double clamp(double x, double lo, double hi);
};
