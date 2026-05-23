#include "Demodulator.h"
using namespace SystemVueModelBuilder;

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(Demodulator)
{
	SET_MODEL_DESCRIPTION("Demodulator");
	SET_MODEL_SYMBOL("SYM_Demodulator");
	SET_MODEL_CATEGORY("Communications");
	SET_MODEL_CATEGORY("Analog/RF");

	{ auto p = ADD_MODEL_INPUT(input);    p.SetDescription("complex envelope input"); }
	{ auto p = ADD_MODEL_OUTPUT(output1); p.SetDescription("output1"); }
	{ auto p = ADD_MODEL_OUTPUT(output2); p.SetDescription("output2"); }

	{ auto p = ADD_MODEL_ENUM_PARAM(OutputType, OutputTypeEnum);
	p.AddEnumeration("I/Q", OT_IQ);
	p.AddEnumeration("Amp/Phase", OT_AmpPhase);
	p.AddEnumeration("Amp/Freq", OT_AmpFreq);
	p.SetDefaultValue("I/Q");
	p.SetDescription("Output type"); }

	{ auto p = ADD_MODEL_PARAM(FCarrier);
	p.SetUnit(Units::FREQUENCY); p.SetDefaultValue("0.2e6");
	p.SetDescription("Carrier frequency"); }

	{ auto p = ADD_MODEL_PARAM(InitialPhase);
	p.SetUnit(Units::ANGLE); p.SetDefaultValue("0");
	p.SetDescription("Initial phase"); }

	{ auto p = ADD_MODEL_PARAM(AmpSensitivity);
	p.SetDefaultValue("1"); p.SetDescription("Amplitude sensitivity"); }

	{ auto p = ADD_MODEL_PARAM(PhaseSensitivity);
	p.SetDefaultValue("1.0/90.0");
	p.SetDescription("Phase deviation sensitivity in Volts/degree");
	p.SetHideCondition("OutputType ~= 1"); }

	{ auto p = ADD_MODEL_PARAM(FreqSensitivity);
	p.SetDefaultValue("1.0e-4");
	p.SetDescription("Frequency deviation sensitivity in Volts/Hz");
	p.SetHideCondition("OutputType ~= 2"); }

	{ auto p = ADD_MODEL_ENUM_PARAM(MirrorSignal, MirrorEnum);
	p.AddEnumeration("NO", Mirror_No);
	p.AddEnumeration("YES", Mirror_Yes);
	p.SetDefaultValue("NO");
	p.SetDescription("Mirror signal about carrier"); }

	{ auto p = ADD_MODEL_ENUM_PARAM(ShowIQ_Impairments, IQImpEnum);
	p.AddEnumeration("NO", IQImp_No);
	p.AddEnumeration("YES", IQImp_Yes);
	p.SetDefaultValue("NO");
	p.SetDescription("Show I and Q impairments"); }

	{ auto p = ADD_MODEL_PARAM(GainImbalance);
	p.SetDefaultValue("0.0");
	p.SetDescription("Gain imbalance in dB");
	p.SetHideCondition("ShowIQ_Impairments ~= 1"); }

	{ auto p = ADD_MODEL_PARAM(PhaseImbalance);
	p.SetUnit(Units::ANGLE); p.SetDefaultValue("0.0");
	p.SetDescription("Phase imbalance");
	p.SetHideCondition("ShowIQ_Impairments ~= 1"); }

	{ auto p = ADD_MODEL_PARAM(I_OriginOffset);
	p.SetDefaultValue("0.0"); p.SetDescription("I origin offset");
	p.SetHideCondition("ShowIQ_Impairments ~= 1"); }

	{ auto p = ADD_MODEL_PARAM(Q_OriginOffset);
	p.SetDefaultValue("0.0"); p.SetDescription("Q origin offset");
	p.SetHideCondition("ShowIQ_Impairments ~= 1"); }

	{ auto p = ADD_MODEL_PARAM(IQ_Rotation);
	p.SetUnit(Units::ANGLE); p.SetDefaultValue("0.0");
	p.SetDescription("IQ rotation");
	p.SetHideCondition("ShowIQ_Impairments ~= 1"); }

	return true;
}
#endif

Demodulator::Demodulator()
	: OutputType(OT_IQ),
	AmpSensitivity(1.0),
	PhaseSensitivity(1.0 / 90.0),
	FreqSensitivity(1.0e-4),
	FCarrier(0.2e6),
	InitialPhase(0.0),
	MirrorSignal(Mirror_No),
	ShowIQ_Impairments(IQImp_No),
	GainImbalance(0.0),
	PhaseImbalance(0.0),
	I_OriginOffset(0.0),
	Q_OriginOffset(0.0),
	IQ_Rotation(0.0)
{}

double Demodulator::unwrapPhase(double rawThetaRad)
{
	if (!havePrev_) return rawThetaRad;
	double d = rawThetaRad - prevThetaRad_;
	const double TWO_PI = 2.0 * kPI;
	while (d > kPI) d -= TWO_PI;
	while (d < -kPI) d += TWO_PI;
	return prevThetaRad_ + d;
}

void Demodulator::processEnvelopeSample(
	const SystemVueModelBuilder::EnvelopeSignal& inSample,
	double tNow,
	double& I_out,
	double& Q_out
) {
	std::complex<double> cx = inSample.complex();

	const double fc_in = input.GetCharacterizationFrequency();
	const double theta0 = deg2rad(InitialPhase);
	const double dFc = fc_in - FCarrier;
	const double ang = 2.0 * kPI * dFc * tNow - theta0;

	cx *= std::complex<double>(std::cos(ang), std::sin(ang));

	cx += std::complex<double>(I_OriginOffset, Q_OriginOffset);
	const double r = deg2rad(IQ_Rotation);
	cx *= std::complex<double>(std::cos(r), std::sin(r));

	double I_raw = cx.real();
	double Q_raw = cx.imag();

	double I_corr = I_raw, Q_corr = Q_raw;
	if (ShowIQ_Impairments == IQImp_Yes) {
		const double gI = std::pow(10.0, (+0.5*GainImbalance / 20.0));
		const double gQ = std::pow(10.0, (-0.5*GainImbalance / 20.0));
		const double phiI = deg2rad(-PhaseImbalance * 0.5);
		const double phiQ = deg2rad(+PhaseImbalance * 0.5);
		I_corr = gI * (I_raw*std::cos(phiI) + Q_raw * std::sin(phiI));
		Q_corr = gQ * (-I_raw * std::sin(phiQ) + Q_raw * std::cos(phiQ));
	}
	if (MirrorSignal == Mirror_Yes) Q_corr = -Q_corr;

	I_out = I_corr;
	Q_out = Q_corr;
}

bool Demodulator::Setup()
{
	output1.SetRate(1U);
	output2.SetRate(1U);
	havePrev_ = false;
	prevThetaRad_ = 0.0;
	prevTime_ = 0.0;
	return true;
}

bool Demodulator::Run()
{
	EnvelopeSignal sNow = input[0U];
	const double tNow = input.GetStartTime() + input.GetTimeStep();

	double I_now = 0.0, Q_now = 0.0;
	processEnvelopeSample(sNow, tNow, I_now, Q_now);

	const double amp = std::hypot(I_now, Q_now);
	const double thetaRaw = std::atan2(Q_now, I_now);
	const double thetaUnwr = unwrapPhase(thetaRaw);

	double freqRadPerSec = 0.0;
	if (havePrev_) {
		const double dth = thetaUnwr - prevThetaRad_;
		const double dt = tNow - prevTime_;
		if (dt > 0.0) freqRadPerSec = dth / dt;
	}

	double y1 = 0.0, y2 = 0.0;
	switch (OutputType) {
	case OT_IQ:
		y1 = AmpSensitivity * I_now;
		y2 = AmpSensitivity * Q_now;
		break;
	case OT_AmpPhase:
		y1 = AmpSensitivity * amp;
		y2 = PhaseSensitivity * (thetaUnwr * 180.0 / kPI);
		break;
	case OT_AmpFreq:
		y1 = AmpSensitivity * amp;
		y2 = FreqSensitivity * (freqRadPerSec / (2.0*kPI));
		break;
	}

	output1[0U] = y1;
	output2[0U] = y2;

	prevThetaRad_ = thetaUnwr;
	prevTime_ = tNow;
	havePrev_ = true;
	return true;
}

