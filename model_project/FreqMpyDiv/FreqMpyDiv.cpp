#include "FreqMpyDiv.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(FreqMpyDiv)
{
	SET_MODEL_DESCRIPTION("Envelope Signal Frequency Multiplier/Divider");
	SET_MODEL_SYMBOL("SYM_FreqMpyDiv");
	SET_MODEL_CATEGORY("Analog/RF");

	{ auto p = ADD_MODEL_INPUT(input);
	p.SetDescription("input signal (envelope)"); }

	{ auto p = ADD_MODEL_INPUT(control);
	p.SetDescription("optional normalized control (envelope)");
	p.SetOptional(); }

	{ auto p = ADD_MODEL_OUTPUT(output);
	p.SetDescription("output signal (envelope)"); }

	{ auto p = ADD_MODEL_ENUM_PARAM(MultDiv, MultDivEnum);
	p.SetUnit(SystemVueModelBuilder::Units::NONE);
	p.AddEnumeration("Multiplier", MD_Multiplier);
	p.AddEnumeration("Divider", MD_Divider);
	p.SetDefaultValue("Multiplier"); }

	{ auto p = ADD_MODEL_PARAM(NominalX);
	p.SetUnit(SystemVueModelBuilder::Units::NONE);
	p.SetDefaultValue("1");
	p.SetDescription("Nominal X for Fc mapping"); }

	{ auto p = ADD_MODEL_PARAM(MaxX);
	p.SetUnit(SystemVueModelBuilder::Units::NONE);
	p.SetDefaultValue("2"); }

	{ auto p = ADD_MODEL_PARAM(MinX);
	p.SetUnit(SystemVueModelBuilder::Units::NONE);
	p.SetDefaultValue("0.5"); }

	{ auto p = ADD_MODEL_ENUM_PARAM(OperatorType, OperatorTypeEnum);
	p.SetUnit(SystemVueModelBuilder::Units::NONE);
	p.AddEnumeration("Full signal", OP_Full);
	p.AddEnumeration("Phase only", OP_PhaseOnly);
	p.SetDefaultValue("Phase only"); }

	return true;
}
#endif

FreqMpyDiv::FreqMpyDiv()
	: MultDiv(MD_Multiplier), NominalX(1.0), MaxX(2.0), MinX(0.5), OperatorType(OP_PhaseOnly)
{
}

bool FreqMpyDiv::PropagateCharacterizationFrequency()
{
	fc_in_ = input.GetCharacterizationFrequency();
	if (MinX <= 0.0) MinX = 1e-6;
	if (MaxX < MinX) MaxX = MinX;
	x_nom_ = clamp(NominalX, MinX, MaxX);
	fc_out_ = g(x_nom_);

	output.SetCharacterizationFrequency(fc_out_);
	return true;
}

bool FreqMpyDiv::Setup()
{
	return PropagateCharacterizationFrequency();
}

bool FreqMpyDiv::Run()
{
	using SystemVueModelBuilder::EnvelopeSignal;

	const EnvelopeSignal xin = input[0U];
	const double tNow = input.GetStartTime() + input.GetTimeStep();

	double X = clamp(NominalX + (control.IsConnected() ? control[0U].real() : 0.0), MinX, MaxX);

	const std::complex<double> cx = xin.complex();
	const double r = std::abs(cx);
	const double th = std::atan2(cx.imag(), cx.real());

	const double Xeff_phase = (MultDiv == MD_Multiplier) ? X : (1.0 / X);
	const double amp_exp = (OperatorType == OP_PhaseOnly) ? 1.0 : Xeff_phase;

	std::complex<double> y = std::polar(std::pow(r, amp_exp), Xeff_phase * th);

	const double dFc = g(X) - fc_out_;
	if (dFc != 0.0 && tNow != 0.0) {
		const double phi = 2.0 * M_PI * dFc * tNow;
		const std::complex<double> rot(std::cos(phi), std::sin(phi));
		y *= rot;
	}

	output[0U] = y;
	return true;
}

