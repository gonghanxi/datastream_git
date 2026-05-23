#include "Modulator.h"
#include <algorithm>

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(Modulator)
{
	SET_MODEL_DESCRIPTION("Modulator");
	SET_MODEL_SYMBOL("SYM_Modulator");
	SET_MODEL_CATEGORY("Analog/RF");

	{ auto p = ADD_MODEL_INPUT(input1);		  p.SetDescription("input1 (real)");  p.SetOptional(); }
	{ auto p = ADD_MODEL_INPUT(input2);       p.SetDescription("input2 (real)");  p.SetOptional(); }
	{ auto p = ADD_MODEL_INPUT(LO);           p.SetDescription("LO (envelope)");  p.SetOptional(); }
	{ auto p = ADD_MODEL_OUTPUT(output);      p.SetDescription("complex envelope output"); }
	{ auto p = ADD_MODEL_OUTPUT(quad_output); p.SetDescription("complex envelope quadrature output"); }

	{
		auto e = ADD_MODEL_ENUM_PARAM(InputType, InputTypeEnum);
		e.AddEnumeration("I/Q", InIQ);
		e.AddEnumeration("Amp/Phase", InAmpPhase);
		e.AddEnumeration("Amp/Freq", InAmpFreq);
		e.SetDefaultValue("I/Q");
		e.SetUnit(SystemVueModelBuilder::Units::NONE);
		e.SetDescription("Input type");
	}
	{
		auto p = ADD_MODEL_PARAM(FCarrier);
		p.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		p.SetDefaultValue("0.2e6");
		p.SetDescription("Carrier frequency (used if optional LO input not used)");
	}
	{
		auto p = ADD_MODEL_PARAM(InitialPhase);
		p.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		p.SetDefaultValue("0");
		p.SetDescription("Initial phase");
	}
	{
		auto p = ADD_MODEL_PARAM(AmpSensitivity);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("1");
		p.SetDescription("Amplitude sensitivity");
	}
	{
		auto p = ADD_MODEL_PARAM(PhaseSensitivity);
		p.SetUnit(SystemVueModelBuilder::Units::NONE); 
		p.SetDefaultValue("90");
		p.SetHideCondition("InputType ~= 1"); 
		p.SetDescription("Phase deviation sensitivity in degrees/Volt");
	}
	{
		auto p = ADD_MODEL_PARAM(FreqSensitivity);
		p.SetUnit(SystemVueModelBuilder::Units::FREQUENCY); 
		p.SetDefaultValue("10000");
		p.SetHideCondition("InputType ~= 2"); 
		p.SetDescription("Frequency deviation sensitivity in Hz/Volt");
	}
	{
		auto e = ADD_MODEL_ENUM_PARAM(ConjugatedQuadrature, ConjQuadEnum);
		e.AddEnumeration("NO", CQ_No);
		e.AddEnumeration("YES", CQ_Yes);
		e.SetDefaultValue("NO");
		e.SetDescription("Define the sign of quadrature output");
	}
	{
		auto e = ADD_MODEL_ENUM_PARAM(MirrorSignal, MirrorEnum);
		e.AddEnumeration("NO", Mirror_No);
		e.AddEnumeration("YES", Mirror_Yes);
		e.SetDefaultValue("NO");
		e.SetDescription("Mirror signal about carrier");
	}

	{
		auto e = ADD_MODEL_ENUM_PARAM(ShowIQ_Impairments, ShowIQEnum);
		e.AddEnumeration("NO", ShowIQ_NO);
		e.AddEnumeration("YES", ShowIQ_YES);
		e.SetDefaultValue("NO");
		e.SetDescription("Show I and Q impairments");
	}

	{
		auto p = ADD_MODEL_PARAM(GainImbalance);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("0.0");
		p.SetHideCondition("ShowIQ_Impairments ~= 1");
		p.SetDescription("Gain imbalance in dB");
	}
	{
		auto p = ADD_MODEL_PARAM(PhaseImbalance);
		p.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		p.SetDefaultValue("0.0");
		p.SetHideCondition("ShowIQ_Impairments ~= 1");
		p.SetDescription("Phase imbalance");
	}
	{
		auto p = ADD_MODEL_PARAM(I_OriginOffset);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("0.0");
		p.SetHideCondition("ShowIQ_Impairments ~= 1");
		p.SetDescription("I origin offset");
	}
	{
		auto p = ADD_MODEL_PARAM(Q_OriginOffset);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("0.0");
		p.SetHideCondition("ShowIQ_Impairments ~= 1");
		p.SetDescription("Q origin offset");
	}
	{
		auto p = ADD_MODEL_PARAM(IQ_Rotation);
		p.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		p.SetDefaultValue("0.0");
		p.SetHideCondition("ShowIQ_Impairments ~= 1");
		p.SetDescription("IQ rotation");
	}

	return true;
}
#endif

Modulator::Modulator()
	: InputType(InIQ),
	FCarrier(0.2e6),
	InitialPhase(0.0),
	AmpSensitivity(1.0),
	PhaseSensitivity(90.0),
	FreqSensitivity(10000.0),
	ConjugatedQuadrature(CQ_No),
	MirrorSignal(Mirror_No),
	ShowIQ_Impairments(ShowIQ_NO),   
	GainImbalance(0.0),
	PhaseImbalance(0.0),
	I_OriginOffset(0.0),
	Q_OriginOffset(0.0),
	IQ_Rotation(0.0),
	phaseAcc_(0.0),
	lastTime_(std::numeric_limits<double>::quiet_NaN())
{}

ERESULT Modulator::PropagateCharacterizationFrequency()
{
	bool bStatus = true;

	double fc = FCarrier;
	if (LO.IsConnected()) {
		fc = LO.GetCharacterizationFrequency();
	}

	if (fc >= 0.0) {
		output.SetCharacterizationFrequency(fc);
		quad_output.SetCharacterizationFrequency(fc);
	}
	else {
		POST_ERROR("characterization frequency must be >= 0.");
		bStatus = false;
	}

	return bStatus;
}

bool Modulator::Setup()
{
	phaseAcc_ = 0.0;
	lastTime_ = std::numeric_limits<double>::quiet_NaN();

	output.SetRate(1U);
	quad_output.SetRate(1U);

	PropagateCharacterizationFrequency();
	return true;
}

bool Modulator::Run()
{
	using SystemVueModelBuilder::EnvelopeSignal;

	const double tNow = output.GetTime(0, 1);
	double dt = 0.0;
	if (std::isfinite(lastTime_)) dt = std::max(0.0, tNow - lastTime_);
	lastTime_ = tNow;

	const bool isAmpType = (InputType != InIQ);
	const bool ampInputConnected = input1.IsConnected();

	const double x1 = ampInputConnected
		? input1[0U]
		: (InputType == InIQ ? 0.0 : 1.0);
	const double x2 = input2.IsConnected() ? input2[0U] : 0.0;

	double Sa_eff = AmpSensitivity;
	if (!ampInputConnected && isAmpType) {
		Sa_eff = 1.0;
	}

	std::complex<double> cx(0.0, 0.0);
	const double th0 = deg2rad(InitialPhase);

	if (InputType == InIQ) {
		cx = std::complex<double>(x1, x2) * AmpSensitivity;
	}
	else if (InputType == InAmpPhase) {
		const double phi = th0 + deg2rad(PhaseSensitivity) * x2;
		const double A = Sa_eff * x1;
		cx = std::polar(A, phi);
	}
	else {
		phaseAcc_ += 2.0 * 3.14159265358979323846 * FreqSensitivity * x2 * dt;
		const double A = Sa_eff * x1;
		cx = std::polar(A, th0 + phaseAcc_);
	}

	if (LO.IsConnected()) {
		cx *= LO[0U].complex();
	}

	if (MirrorSignal == Mirror_Yes) {
		cx = std::conj(cx);
	}

	if (ShowIQ_Impairments == ShowIQ_YES) {   
		const double gI = std::pow(10.0, (+0.5 * GainImbalance) / 20.0);
		const double gQ = std::pow(10.0, (-0.5 * GainImbalance) / 20.0);
		const double phiI = deg2rad(-PhaseImbalance * 0.5);
		const double phiQ = deg2rad(+PhaseImbalance * 0.5);

		const double I = cx.real(), Q = cx.imag();
		const double Ip = gI * I * std::cos(phiI) - gQ * Q * std::sin(phiQ);
		const double Qp = gI * I * std::sin(phiI) + gQ * Q * std::cos(phiQ);

		std::complex<double> cx_imp(Ip, Qp);
		cx_imp *= std::exp(std::complex<double>(0.0, deg2rad(IQ_Rotation)));
		cx_imp += std::complex<double>(I_OriginOffset, Q_OriginOffset);

		cx = cx_imp;
	}

	output[0U] = EnvelopeSignal(cx);

	std::complex<double> quad;
	if (ConjugatedQuadrature == CQ_No) {
		quad = std::complex<double>(-cx.imag(), cx.real());
	}
	else {
		quad = std::complex<double>(cx.imag(), -cx.real());
	}
	quad_output[0U] = EnvelopeSignal(quad);

	return true;
}
