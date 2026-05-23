#pragma once
#include "ModelBuilder.h"
#include "DFModel.h"
#include "TimedCircularBuffer.h"
#include "EnvelopeSignal.h"
#include "SystemVue.h"
#include <complex>
#include <cmath>

class SYSTEMVUEMODELBUILDER_API Demodulator : public SystemVueModelBuilder::DFModel
{
public:
	enum OutputTypeEnum { OT_IQ = 0, OT_AmpPhase = 1, OT_AmpFreq = 2 };
	enum MirrorEnum { Mirror_No = 0, Mirror_Yes = 1 };
	enum IQImpEnum { IQImp_No = 0, IQImp_Yes = 1 };

	DECLARE_MODEL_INTERFACE(Demodulator);
	Demodulator();

	bool Setup() override;
	bool Run() override;

	SystemVueModelBuilder::EnvelopeCircularBuffer      input;
	SystemVueModelBuilder::TimedCircularBuffer<double> output1;
	SystemVueModelBuilder::TimedCircularBuffer<double> output2;

	OutputTypeEnum OutputType;
	double AmpSensitivity;
	double PhaseSensitivity;
	double FreqSensitivity;
	double FCarrier;
	double InitialPhase;
	MirrorEnum  MirrorSignal;
	IQImpEnum   ShowIQ_Impairments;
	double GainImbalance;
	double PhaseImbalance;
	double I_OriginOffset;
	double Q_OriginOffset;
	double IQ_Rotation;

	static constexpr double kPI = 3.14159265358979323846;
	static inline double deg2rad(double d) { return d * (kPI / 180.0); }

	double prevThetaRad_{ 0.0 };
	double prevTime_{ 0.0 };
	bool   havePrev_{ false };

	double unwrapPhase(double rawThetaRad);
	void processEnvelopeSample(const SystemVueModelBuilder::EnvelopeSignal& sNow,
		double tNow, double& I_out, double& Q_out);

private:
};
