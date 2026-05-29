#include "Diagonal_M.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(Diagonal_M)
{
	SET_MODEL_DESCRIPTION("Diagonal Matrix Generator");
	SET_MODEL_SYMBOL("SYM_Diagonal_M");
	SET_MODEL_CATEGORY("Math Matrix");

	ADD_MODEL_OUTPUT(output);

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(DiagonalElements);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("[1, 2]");
		p.SetDescription("Output matrix diagonal elements");
	}

	{
		SystemVueModelBuilder::DFParam p =
			ADD_MODEL_ENUM_PARAM(ShowAdvancedParams, ShowAdvancedEnum);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.AddEnumeration("NO", ShowAdv_No);
		p.AddEnumeration("YES", ShowAdv_Yes);
		p.SetDefaultValue("NO");
		p.SetDescription("Show advanced parameters");
	}

	{
		SystemVueModelBuilder::DFParam p =
			ADD_MODEL_ENUM_PARAM(SampleRateOption, SampleRateOptionEnum);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.AddEnumeration("UnTimed", SRO_UnTimed);
		p.AddEnumeration("Timed from SampleRate", SRO_TimedFromSampleRate);
		p.AddEnumeration("Timed from Schematic", SRO_TimedFromSchematic);
		p.SetDefaultValue("Timed from Schematic");
		p.SetDescription("Sample rate option");
		p.SetHideCondition("ShowAdvancedParams ~= 1"); 
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(SampleRate);
		p.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		p.SetDefaultValue("Sample_Rate");           
		p.SetDescription("Explicit sample rate");
		p.SetHideCondition("ShowAdvancedParams ~= 1 || SampleRateOption ~= 1");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(InitialDelay);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("0");
		p.SetDescription("Output sample delay");
		p.SetDynamicUpdate(true);
		p.SetHideCondition("ShowAdvancedParams ~= 1");
	}
	return true;
}
#endif 

Diagonal_M::Diagonal_M()
	: N_(0), produced_(0)
{
	DiagonalElements.Resize(1, 2);
	DiagonalElements(0) = 1.0;
	DiagonalElements(1) = 2.0;

	ShowAdvancedParams = ShowAdv_No;
	SampleRateOption = SRO_TimedFromSchematic;
	SampleRate = 1.0e6; 
	InitialDelay = 0;
}

bool Diagonal_M::Setup()
{
	N_ = static_cast<int>(DiagonalElements.NumElements());
	if (N_ <= 0) {
		POST_ERROR("Diagonal_M: DiagonalElements must contain at least one element.");
		return false;
	}

	diagMat_.Resize(N_, N_);
	zeroMat_.Resize(N_, N_);
	for (int r = 0; r < N_; ++r) {
		for (int c = 0; c < N_; ++c) {
			diagMat_(r, c) = (r == c) ? DiagonalElements(r) : 0.0;
			zeroMat_(r, c) = 0.0;
		}
	}
	produced_ = 0;

	const bool adv = (ShowAdvancedParams == ShowAdv_Yes);
	const bool isUnTimed = adv && (SampleRateOption == SRO_UnTimed);
	const bool isTimedFromSR = adv && (SampleRateOption == SRO_TimedFromSampleRate);

	if (isTimedFromSR) {
		if (SampleRate <= 0.0) {
			POST_ERROR("Diagonal_M: SampleRate must be > 0 when SampleRateOption = Timed from SampleRate.");
			return false;
		}
		output.SetSampleRate(SampleRate);
	}
	if (adv && InitialDelay < 0) {
		POST_ERROR("Diagonal_M: InitialDelay must be >= 0.");
		return false;
	}

#ifdef SV_CODE_GEN
	// ====== CodeGen Ô¼Êø£º½ö UnTimed ÔÊÐí ======
	if (!isUnTimed) {
		POST_ERROR("C++ code generation is supported only when SampleRateOption equals to UnTimed.");
		return false;
	}
#endif

	return true;
}

bool Diagonal_M::Run()
{
	const bool adv = (ShowAdvancedParams == ShowAdv_Yes);

	const SystemVueModelBuilder::Matrix<double>& y =
		(adv && (produced_ < InitialDelay)) ? zeroMat_ : diagMat_;

	output[0] = y;

	++produced_;
	return true;
}
