#pragma once

#include "ModelBuilder.h"
#include "TimedDFModel.h"
#include "EnvelopeSignal.h"

#include <complex>
#include <vector>
#include <deque>

class SYSTEMVUEMODELBUILDER_API RADAR_DDC : public SystemVueModelBuilder::TimedDFModel
{
public:
	DECLARE_MODEL_INTERFACE(RADAR_DDC);

	RADAR_DDC();

	virtual bool Setup();
	virtual bool Run();
	virtual ERESULT PropagateCharacterizationFrequency();

	typedef std::complex<double> Cx;

	// ============================================================
	// 端口定义
	// 端口 1：IF_Signal，envelope 中频包络输入
	// 端口 2：BB_Signal，complex 基带复信号输出
	// ============================================================
	SystemVueModelBuilder::EnvelopeCircularBuffer IF_Signal;
	SystemVueModelBuilder::DComplexCircularBuffer BB_Signal;

	// ============================================================
	// RADAR_DDC 帮助文档参数
	// ============================================================
	double IF_Freq;
	double IF_SamplingRate;
	int    ADC_NBits;
	double PhaseImbalance;
	int    BB_DownSamplingRatio;
	double RC_ExcessBW;
	double Out_CenterFreq;

private:
	// ============================================================
	// QuadSample / RC 抽取滤波状态
	// ============================================================
	std::vector<double> quadFir_;
	std::deque<Cx>      quadFirState_;

	double inputSampleRateHz_;
	double inputTimeStepSec_;
	double outputSampleRateHz_;
	double outputTimeStepSec_;

	int decim_;

private:
	void applyRates_();
	void resetStates_();

	void buildQuadSampleFir_();
	Cx   runQuadSampleFir_(const Cx& x);

	Cx envelopeToComplex_(const SystemVueModelBuilder::EnvelopeSignal& x,
		double fcHz) const;

	double envelopeToRealIF_(const SystemVueModelBuilder::EnvelopeSignal& x,
		double inputFcHz,
		double timeNow) const;

	double applyADC_(double x) const;

	Cx quadSampleOneIFPoint_(double realIf,
		double timeNow) const;

	Cx applyOutCenterFreq_(const Cx& x,
		double timeNow) const;

	static double raisedCosineImpulse_(double t, double alpha);
	static double sinc_(double x);
	static double deg2rad(double x);
	static double clamp(double x, double lo, double hi);
};
