#pragma once

#include "ModelBuilder.h"
#include "TimedDFModel.h"

#include <complex>
#include <vector>
#include <deque>

class RADAR_QuadSample : public SystemVueModelBuilder::TimedDFModel
{
public:
	DECLARE_MODEL_INTERFACE(RADAR_QuadSample);

	RADAR_QuadSample();

	virtual bool Setup();
	virtual bool Run();
	virtual ERESULT PropagateCharacterizationFrequency();

	typedef std::complex<double> Cx;

	// ============================================================
	// 端口定义
	// Port 1：IF_Signal，real 数字中频实信号输入
	// Port 2：BB_Signal，complex 基带复信号输出
	// ============================================================
	SystemVueModelBuilder::CircularBuffer<double> IF_Signal;
	SystemVueModelBuilder::DComplexCircularBuffer BB_Signal;

	// ============================================================
	// RADAR_QuadSample 帮助文档参数
	// ============================================================
	int    BB_DownSamplingRatio;
	double IF_Freq;
	double IF_SamplingRate;
	double Out_CenterFreq;
	double PhaseImbalance;
	double RC_ExcessBW;

private:
	// ============================================================
	// FIR_Cx / DownSample 状态
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

	Cx quadSampleOneIFPoint_(double realIf,
		double timeNow) const;

	Cx applyOutCenterFreq_(const Cx& x,
		double timeNow) const;

	static double raisedCosineImpulse_(double t, double alpha);
	static double sinc_(double x);
	static double deg2rad(double x);
	static double clamp(double x, double lo, double hi);
};
