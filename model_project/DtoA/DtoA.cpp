#include "DtoA.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(DtoA)
{
	SET_MODEL_DESCRIPTION("Digital to Analog Converter");
	SET_MODEL_SYMBOL("SYM_DtoA");
	SET_MODEL_CATEGORY("Analog/RF");

	{ auto p = ADD_MODEL_INPUT(D); p.SetDescription("input to D/A (integer)"); }
	{ auto p = ADD_MODEL_OUTPUT(A); p.SetDescription("output of D/A (real)"); }

	{ auto p = ADD_MODEL_PARAM(NBits); p.SetDefaultValue("8"); }
	{ auto p = ADD_MODEL_PARAM(VRef);  p.SetUnit(SystemVueModelBuilder::Units::VOLTAGE); p.SetDefaultValue("1.0"); }
	{ auto p = ADD_MODEL_ENUM_PARAM(InputDigitalFormat, DigFmt);
	p.AddEnumeration("Offset binary", OffsetBinary);
	p.AddEnumeration("Twos-complement", TwosComplement);
	p.SetDefaultValue("Twos-complement"); }
	{ auto p = ADD_MODEL_PARAM(RepeatOutput); p.SetDefaultValue("1"); }
	{ auto p = ADD_MODEL_PARAM(RJrms); p.SetUnit(SystemVueModelBuilder::Units::TIME); p.SetDefaultValue("0.0"); }
	{ auto p = ADD_MODEL_PARAM(INL); p.SetDefaultValue("0.0"); }
	{ auto p = ADD_MODEL_PARAM(DNL); p.SetDefaultValue("0.0"); }
	{ auto p = ADD_MODEL_ENUM_PARAM(HarmonicDistortion, HDist);
	p.AddEnumeration("None", HD_None);
	p.AddEnumeration("Basic Distortion", HD_Basic);
	p.AddEnumeration("Settable with Data Table", HD_Table);
	p.SetDefaultValue("None"); }

	{
		auto p = ADD_MODEL_ENUM_PARAM(dBcReference, DbRef);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.AddEnumeration("Signal Fo to dBFS", Ref_SignalFo_to_DBFS);  // 0
		p.AddEnumeration("Signal Fo only", Ref_SignalFo_only);    // 1
		p.SetDefaultValue("Signal Fo to dBFS");
		p.SetHideCondition("HarmonicDistortion ~= 2");  
	}

	{
		auto p = ADD_MODEL_PARAM(dBFS);
		p.SetDefaultValue("-1.0");
		p.SetHideCondition("HarmonicDistortion ~= 2 || dBcReference ~= 0");
	}

	{ auto p = ADD_MODEL_PARAM(F2_to_F5_dBc); p.SetDefaultValue("[-400, -400, -400, -400]");
	p.SetHideCondition("HarmonicDistortion == 0"); }

	{ auto p = ADD_MODEL_PARAM(C1_to_C5_dB);  p.SetDefaultValue("[-400, -400, -400, -400, -400]");
	p.SetHideCondition("HarmonicDistortion == 0"); }

	{ auto p = ADD_MODEL_PARAM(DataTable);
	p.SetDefaultValue("[1, 3, -400, 0; 1, -3, -400, 0]");
	p.SetHideCondition("HarmonicDistortion ~= 2"); }

	{ auto p = ADD_MODEL_PARAM(FundamentalFo);
	p.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
	p.SetDefaultValue("100e6");
	p.SetHideCondition("HarmonicDistortion ~= 2"); }

	{ auto p = ADD_MODEL_PARAM(SetPhase);
	p.SetDefaultValue("0");
	p.SetHideCondition("HarmonicDistortion ~= 2"); }

	return true;
}
#endif

DtoA::DtoA()
	: NBits(8), VRef(1.0), InputDigitalFormat(TwosComplement),
	RepeatOutput(1), RJrms(0.0), INL(0.0), DNL(0.0),
	HarmonicDistortion(HD_None),
	dBFS(-1.0), dBcReference(Ref_SignalFo_to_DBFS),
	FundamentalFo(100e6), SetPhase(0),
	rng_(1234567), gauss0_(0.0, 1.0)
{
	F2_to_F5_dBc.Resize(1, 4); for (int i = 0; i < 4; ++i) F2_to_F5_dBc(i) = -400.0;
	C1_to_C5_dB.Resize(1, 5);  for (int i = 0; i < 5; ++i) C1_to_C5_dB(i) = -400.0;
	DataTable.Resize(2, 4);
	DataTable(0, 0) = 1; DataTable(0, 1) = 3; DataTable(0, 2) = -400.0; DataTable(0, 3) = 0.0;
	DataTable(1, 0) = 1; DataTable(1, 1) = -3; DataTable(1, 2) = -400.0; DataTable(1, 3) = 0.0;
}

bool DtoA::Setup()
{
	Fc_ = D.GetSampleRate(); if (Fc_ <= 0.0) Fc_ = 1.0;
	FsOut_ = std::max(1, RepeatOutput) * Fc_;
	A.SetSampleRate(FsOut_);

	if (InputDigitalFormat == OffsetBinary) { codeMin_ = 0; codeMax_ = (1 << NBits) - 1; }
	else { codeMin_ = -(1 << (NBits - 1)); codeMax_ = (1 << (NBits - 1)) - 1; }
	LSB_ = 2.0*VRef / double(1 << NBits);

	buildQuantTable_();
	parseDataTable_();

	const double dt = 1.0 / FsOut_; (void)dt;
	alpha_ = 0.999;   

	produced_ = 0; Afo_est_ = 1e-12; i_est_ = q_est_ = 0.0;
	return true;
}

void DtoA::buildQuantTable_()
{
	const int Ncode = codeMax_ - codeMin_ + 1;
	code2volt_.assign(Ncode, 0.0);
	for (int c = codeMin_; c <= codeMax_; ++c) {
		int idx = c - codeMin_;
		double mid = -VRef + (idx + 0.5)*LSB_;
		code2volt_[idx] = clip(mid, -VRef, VRef);
	}
	if (DNL > 0.0 || INL > 0.0) {
		for (int i = 0; i < Ncode; ++i) {
			double wig = (DNL > 0.0 ? gauss0_(rng_)*0.25*DNL*LSB_ : 0.0)
				+ (INL > 0.0 ? 0.5*INL*LSB_*std::sin(2 * kPI*(i + 0.5) / Ncode) : 0.0);
			code2volt_[i] = clip(code2volt_[i] + wig, -VRef, VRef);
		}
	}
}

double DtoA::codeToVolt_(int code) const
{
	code = clip(code, codeMin_, codeMax_);
	return code2volt_[code - codeMin_];
}

void DtoA::parseDataTable_()
{
	terms_.clear();
	const int R = (int)DataTable.NumRows();
	const int C = (int)DataTable.NumColumns();
	if (R <= 0 || C < 3) return;
	for (int r = 0; r < R; ++r) {
		Term t{};
		t.N = (int)std::floor(DataTable(r, 0) + 0.5);
		t.M = (int)std::floor(DataTable(r, 1) + 0.5);
		t.level_dBc = DataTable(r, 2);
		t.phase_deg = (C >= 4 ? DataTable(r, 3) : 0.0);
		terms_.push_back(t);
	}
}

double DtoA::applyRJ_(double y_now, double y_prev, double dt) const
{
	if (RJrms <= 0.0) return y_now;
	const double jitter = gauss0_(rng_) * RJrms;
	const double dydt = (dt > 0.0) ? (y_now - y_prev) / dt : 0.0;
	return clip(y_now + dydt * jitter, -VRef, VRef);
}

double DtoA::basicHarmonics_(double y) const
{
	if (HarmonicDistortion != HD_Basic) return y;
	double u = (VRef > 0.0) ? clip(y / VRef, -1.0, 1.0) : 0.0;
	const double T1 = u, T2 = 2 * u*u - 1, T3 = 2 * u*T2 - T1, T4 = 2 * u*T3 - T2, T5 = 2 * u*T4 - T3;

	const double aref = VRef * db2lin(dBFS);
	const double a1 = (std::fabs(aref) > 1e-12 ? std::fabs(y) / aref : 0.0);
	const double h2 = db2lin(F2_to_F5_dBc(0)) * std::pow(a1, 1.0);
	const double h3 = db2lin(F2_to_F5_dBc(1)) * std::pow(a1, 2.0);
	const double h4 = db2lin(F2_to_F5_dBc(2)) * std::pow(a1, 3.0);
	const double h5 = db2lin(F2_to_F5_dBc(3)) * std::pow(a1, 4.0);

	double y2 = y
		+ VRef * (-h2)*T2
		+ VRef * (-h3)*T3
		+ VRef * (-h4)*T4
		+ VRef * (-h5)*T5;
	return clip(y2, -VRef, VRef);
}

double DtoA::clockHarmonics_(double t) const
{
	if (HarmonicDistortion == HD_None) return 0.0;
	const double twopi = 2.0 * kPI;
	double acc = 0.0;
	for (int n = 1; n <= 5; ++n) {
		double amp = VRef * db2lin(C1_to_C5_dB(n - 1));
		if (amp <= 0) continue;
		acc += amp * std::sin(twopi * (n*Fc_) * t); 
	}
	return acc;
}

double DtoA::tableTerm_(const Term& t, double t_now) const
{
	const double twopi = 2.0 * kPI;
	const double f = t.N*Fc_ + t.M*FundamentalFo;
	double amp = 0.0;
	if (dBcReference == Ref_SignalFo_to_DBFS) amp = dBcAbs_SignalFoToDBFS_(t.level_dBc);
	else                                      amp = dBcAbs_SignalFoOnly_(t.level_dBc);
	const double ph = (SetPhase ? t.phase_deg * (twopi / 360.0) : 0.0);
	return amp * std::sin(twopi * f * t_now + ph);
}

bool DtoA::Run()
{
	int d = D[0U];
	d = clip(d, codeMin_, codeMax_);
	const double a_quant = codeToVolt_(d);

	const double dt = 1.0 / FsOut_;

	static double a_prev = 0.0;
	double a_base = applyRJ_(a_quant, a_prev, dt);
	a_prev = a_quant;

	for (int k = 0; k < RepeatOutput; ++k) {
		const double t = A.GetStartTime() + static_cast<double>(produced_ + k) * A.GetTimeStep();

		double y = a_base;
		if (HarmonicDistortion == HD_Basic) {
			y = basicHarmonics_(y);
		}

		if (HarmonicDistortion == HD_Table) {
			double c = std::cos(2.0*kPI*FundamentalFo*t);
			double s = std::sin(2.0*kPI*FundamentalFo*t);
			i_est_ = alpha_ * i_est_ + (1.0 - alpha_)* (a_base * c);
			q_est_ = alpha_ * q_est_ + (1.0 - alpha_)* (a_base * s);
			Afo_est_ = std::max(1e-12, std::sqrt(i_est_*i_est_ + q_est_ * q_est_));

			double sum = 0.0;
			for (const auto& term : terms_) sum += tableTerm_(term, t);
			y = clip(a_base + sum, -VRef, VRef);
		}

		if (HarmonicDistortion != HD_None) {
			y = clip(y + clockHarmonics_(t), -VRef, VRef);
		}

		A[k] = y;
	}

	produced_++;
	return true;
}

