#include "Convolve.h"
#include <algorithm> 

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(Convolve)
{
	using namespace SystemVueModelBuilder;

	SET_MODEL_DESCRIPTION("Convoluton Functon");
	SET_MODEL_SYMBOL("SYM_Convolve");
	SET_MODEL_CATEGORY("Signal Processing");

	{
		DFPort p = ADD_MODEL_INPUT(inA);
		p.SetDescription("inA");
	}
	{
		DFPort p = ADD_MODEL_INPUT(inB);
		p.SetDescription("inB");
	}
	{
		DFPort p = ADD_MODEL_OUTPUT(out);
		p.SetDescription("out");
	}

	{
		DFParam p = ADD_MODEL_PARAM(TruncationDepth);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("256");  
		p.SetDescription("Maximum number of terms in convolution sum");
	}

	return true;
}
#endif  

Convolve::Convolve()
	: TruncationDepth(256),
	iter_(0)
{
}

bool Convolve::Setup()
{
	inA.SetRate(1U);
	inB.SetRate(1U);
	out.SetRate(1U);

	if (TruncationDepth <= 0)
	{
		POST_ERROR("Convolve: TruncationDepth must be > 0.");
        LOG_ERROR("Convolve: TruncationDepth must be > 0.");
		return false;
	}

	histA_.clear();
	histB_.clear();
	iter_ = 0;

	return true;
}

bool Convolve::Run()
{
	const double a_n = inA[0];
	const double b_n = inB[0];

	histA_.push_back(a_n);
	histB_.push_back(b_n);

	const unsigned long long n = iter_;
	const std::size_t lenA = histA_.size();
	const std::size_t lenB = histB_.size();

	const int maxTerms = TruncationDepth;

	double acc = 0.0;
	int    termsUsed = 0;

	for (unsigned long long k = 0; k <= n; ++k)
	{
		if (termsUsed >= maxTerms)
			break;

		const std::size_t idxA = static_cast<std::size_t>(k);
		const std::size_t idxB = static_cast<std::size_t>(n - k);

		if (idxA >= lenA || idxB >= lenB)
			continue;   

		acc += histA_[idxA] * histB_[idxB];
		++termsUsed;
	}

	out[0] = acc;

	++iter_;

	return true;
}
