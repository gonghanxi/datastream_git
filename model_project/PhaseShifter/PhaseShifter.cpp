#include "PhaseShifter.h"
#include <algorithm>

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(PhaseShifter)
{
	SET_MODEL_DESCRIPTION("Phase Shifter");
	SET_MODEL_SYMBOL("SYM_PhaseShifter");
	SET_MODEL_CATEGORY("Analog/RF");

	{ auto p = ADD_MODEL_INPUT(input);
	p.SetDescription("input signal (envelope)"); }

	{ auto p = ADD_MODEL_INPUT(control);
	p.SetDescription("optional control signal (real)");
	p.SetOptional(); }                            

	{ auto p = ADD_MODEL_OUTPUT(output);
	p.SetDescription("output signal (envelope)"); }

	{ auto p = ADD_MODEL_PARAM(PhaseShift);
	p.SetUnit(SystemVueModelBuilder::Units::ANGLE);
	p.SetDefaultValue("0");
	p.SetDescription("Phase shift angle (used when the optional control input is not used)"); }

	{ auto p = ADD_MODEL_PARAM(InsertionLoss);
	p.SetUnit(SystemVueModelBuilder::Units::POWER);
	p.SetDefaultValue("0");
	p.SetDescription("Insertion loss"); }

	{ auto p = ADD_MODEL_ENUM_PARAM(Quantization, QuantEnum);
	p.SetUnit(SystemVueModelBuilder::Units::NONE);
	p.AddEnumeration("NO", Quant_NO);
	p.AddEnumeration("Number of Bits (Uniform)", Quant_Bits);
	p.AddEnumeration("Custom Levels", Quant_Custom);
	p.SetDefaultValue("NO");
	p.SetDescription("Quantize PhaseShift value"); }

	{ auto p = ADD_MODEL_PARAM(NumBits);
	p.SetUnit(SystemVueModelBuilder::Units::NONE);
	p.SetDefaultValue("6");
	p.SetDescription("Number of bits for quantization");
	p.SetHideCondition("Quantization ~= 1"); }

	{ auto p = ADD_MODEL_PARAM(Levels);
	p.SetUnit(SystemVueModelBuilder::Units::ANGLE);
	p.SetDescription("Quantization Levels");
	p.SetHideCondition("Quantization ~= 2"); }

	{ auto p = ADD_MODEL_ENUM_PARAM(PhaseShiftError, ErrEnum);
	p.SetUnit(SystemVueModelBuilder::Units::NONE);
	p.AddEnumeration("None", Err_None);
	p.AddEnumeration("Normal", Err_Normal);
	p.AddEnumeration("Uniform", Err_Uniform);
	p.AddEnumeration("Custom Error", Err_Custom);
	p.SetDefaultValue("None");
	p.SetDescription("Error distribution for PhaseShift value"); }

	{ auto p = ADD_MODEL_PARAM(StdDev);
	p.SetUnit(SystemVueModelBuilder::Units::ANGLE);
	p.SetDefaultValue("3.0");
	p.SetDescription("Standard deviation of normal distribution for PhaseShiftError");
	p.SetHideCondition("PhaseShiftError ~= 1"); }

	{ auto p = ADD_MODEL_PARAM(Min);
	p.SetUnit(SystemVueModelBuilder::Units::ANGLE);
	p.SetDefaultValue("-5.0");
	p.SetDescription("Minimum PhaseShiftError of uniform distribution");
	p.SetHideCondition("PhaseShiftError ~= 2"); }

	{ auto p = ADD_MODEL_PARAM(Max);
	p.SetUnit(SystemVueModelBuilder::Units::ANGLE);
	p.SetDefaultValue("5.0");
	p.SetDescription("Maximum PhaseShiftError of uniform distribution");
	p.SetHideCondition("PhaseShiftError ~= 2"); }

	{ auto p = ADD_MODEL_PARAM(CustomError);
	p.SetUnit(SystemVueModelBuilder::Units::ANGLE);
	p.SetDefaultValue("0.0");
	p.SetDescription("User-defined phase shift error, specified in PhaseUnit");
	p.SetHideCondition("PhaseShiftError ~= 3"); }

	{ auto p = ADD_MODEL_PARAM(Sensitivity);
	p.SetUnit(SystemVueModelBuilder::Units::ANGLE);
	p.SetDefaultValue("90");
	p.SetDescription("Phase shift sensitivity in angle/Volt (used when the optional control input is used)"); }

	{ auto p = ADD_MODEL_PARAM(HilbertFilterLength);
	p.SetUnit(SystemVueModelBuilder::Units::NONE);
	p.SetDefaultValue("64");
	p.SetDescription("Hilbert filter sample length (used when input is a real signal)"); }

	return true;
}
#endif

PhaseShifter::PhaseShifter()
	: PhaseShift(0.0)
	, InsertionLoss(0.0)
	, Quantization(Quant_NO)
	, NumBits(6)
	, PhaseShiftError(Err_None)
	, CustomError(0.0)
	, StdDev(3.0)
	, Min(-5.0)
	, Max(5.0)
	, Sensitivity(90.0)
	, HilbertFilterLength(64)
{
	Levels.Resize(1, 0); 
}

ERESULT PhaseShifter::PropagateCharacterizationFrequency()
{
	bool bStatus = true;

	double fc = input.GetCharacterizationFrequency();
	if (fc >= 0.0) {
		output.SetCharacterizationFrequency(fc);
	}
	else {
		POST_ERROR("characterization frequency must be >= 0.");
		bStatus = false;
	}
	return bStatus;
}

void PhaseShifter::buildHilbert(int Lin)
{
	L_ = Lin;
	if (L_ < 3) L_ = 3;
	if ((L_ % 2) == 0) ++L_;
	h_.assign(L_, 0.0);

	const int M = (L_ - 1) / 2; // ÖÐÐÄ
	for (int n = 0; n < L_; ++n) {
		int m = n - M;
		if (m == 0) { h_[n] = 0.0; continue; }
		if ((m & 1) != 0) {
			h_[n] = 2.0 / (kPI * double(m));
		}
		else {
			h_[n] = 0.0;
		}
	}
	x_.clear();
	x_.resize(L_, 0.0);
}

double PhaseShifter::hilbertConv() const
{
	double acc = 0.0;
	const int L = L_;
	for (int k = 0; k < L; ++k) {
		const double xnk = x_[L - 1 - k];
		acc += h_[k] * xnk;
	}
	return acc;
}

double PhaseShifter::delayedReal() const
{
	const int d = L_ / 2;
	const int idx = std::max(0, int(x_.size()) - 1 - d);
	return x_[idx];
}

bool PhaseShifter::Setup()
{
	(void)PropagateCharacterizationFrequency();

	const double fc = input.GetCharacterizationFrequency();
	if (fc == 0.0) {
		buildHilbert(HilbertFilterLength);
	}
	output.SetRate(1U);
	return true;
}

double PhaseShifter::computePhaseRad(double baseDeg)
{
	double D = baseDeg;

	if (Quantization == Quant_Bits && NumBits > 0) {
		const double step = 360.0 / double(1 << NumBits);
		double q = D / step;
		double n = std::floor(q);
		double frac = q - n;
		if (frac > 0.5 - 1e-15) n += 1.0;
		D = n * step;
	}
	else if (Quantization == Quant_Custom && Levels.NumElements() > 0) {
		double best = Levels(0);
		double bestDiff = std::fabs(D - best);
		for (int i = 1; i < (int)Levels.NumElements(); ++i) {
			double v = Levels(i);
			double diff = std::fabs(D - v);
			if (diff < bestDiff || (std::fabs(diff - bestDiff) < 1e-15 && v > best)) {
				best = v; bestDiff = diff;
			}
		}
		D = best;
	}

	if (PhaseShiftError == Err_Normal) {
		std::normal_distribution<double> dist(0.0, std::fabs(StdDev));
		D += dist(rngN_);
	}
	else if (PhaseShiftError == Err_Uniform) {
		double a = std::min(Min, Max), b = std::max(Min, Max);
		std::uniform_real_distribution<double> dist(a, b);
		D += dist(rngU_);
	}
	else if (PhaseShiftError == Err_Custom) {
		D += CustomError;
	}

	return D * kPI / 180.0; // deg -> rad
}

bool PhaseShifter::Run()
{
	using SystemVueModelBuilder::EnvelopeSignal;

	EnvelopeSignal xin = input[0U];
	const double fc = input.GetCharacterizationFrequency();

	double baseDeg = PhaseShift;
	if (control.IsConnected()) {
		baseDeg = Sensitivity * control[0U];
	}
	const double phi = computePhaseRad(baseDeg);
	const double c = std::cos(phi), s = std::sin(phi);
	const double A = ampScale();

	EnvelopeSignal y;

	if (fc > 0.0) {
		auto cx = xin.complex();
		auto rot = std::complex<double>(c, s);
		cx *= rot;
		cx *= std::complex<double>(A, 0.0);
		y = cx;
	}
	else {
		const double v = xin.real();
		x_.push_back(v);
		if ((int)x_.size() > L_) x_.pop_front();   

		const double v1h = hilbertConv();
		const double v1d = delayedReal();

		const double v2 = (v1d * c - v1h * s) * A;
		y = v2;
	}

	output[0U] = y;
	return true;
}
