#include "AutoCorr.h"
#include <cmath>    

using namespace SystemVueModelBuilder;

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(AutoCorr)
{
	using SystemVueModelBuilder::DFParam;

	SET_MODEL_DESCRIPTION("AutoCorrelation Estimator");
	SET_MODEL_SYMBOL("SYM_AutoCorr");
	SET_MODEL_CATEGORY("Signal Processing");

	{
		auto p = ADD_MODEL_INPUT(input);
		p.SetDescription("Input signal");
	}
	{
		auto p = ADD_MODEL_OUTPUT(output);
		p.SetDescription("Autocorrelation output");
	}

	{
		DFParam p = ADD_MODEL_ENUM_PARAM(m_CorrelationType, CorrelationType);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.AddEnumeration("NonCircular", NonCircular);
		p.AddEnumeration("Circular", Circular);
		p.SetDefaultValue("Circular");   
		p.SetDescription("Correlation method: NonCircular, Circular");
		p.SetDynamicUpdate(true);        
	}

	{
		DFParam p = ADD_MODEL_PARAM(CorrelationLength);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("500");
		p.SetDescription("Number of input samples");
	}

	{
		DFParam p = ADD_MODEL_PARAM(StartLag);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("-50");
		p.SetDescription("Low lag limit to output");
	}

	{
		DFParam p = ADD_MODEL_PARAM(StopLag);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("50");
		p.SetDescription("High lag limit to output");
	}

	{
		DFParam p = ADD_MODEL_ENUM_PARAM(m_Normalization, Normalization);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.AddEnumeration("None", None);
		p.AddEnumeration("UnBiased", UnBiased);
		p.AddEnumeration("Biased", Biased);
		p.SetDefaultValue("None");   
		p.SetDescription("Correlation estimate normalization: None, UnBiased, Biased");
		p.SetDynamicUpdate(true);    
	}

	return true;
}
#endif 

AutoCorr::AutoCorr()
	: m_CorrelationType(Circular),
	CorrelationLength(500),
	StartLag(-50),
	StopLag(50),
	m_Normalization(None),
	m_numLags(0)
{
}

bool AutoCorr::Setup()
{
	if (CorrelationLength <= 0) {
		POST_ERROR("AutoCorr: CorrelationLength must be > 0.");
		return false;
	}

	if (StopLag < StartLag) {
		POST_ERROR("AutoCorr: StopLag must be >= StartLag.");
		return false;
	}

	m_numLags = StopLag - StartLag + 1;
	if (m_numLags <= 0) {
		POST_ERROR("AutoCorr: StopLag - StartLag + 1 must be > 0.");
		return false;
	}

	const int N = CorrelationLength;
	if (StartLag < -(N - 1) || StopLag >(N - 1)) {
		POST_WARNING("AutoCorr: |lag| larger than CorrelationLength-1; "
			"NonCircular estimate will be based on fewer overlapping samples.");
	}

	input.SetRate(static_cast<size_t>(N));
	output.SetRate(static_cast<size_t>(m_numLags));

	m_samples.assign(static_cast<size_t>(N), 0.0);

	return true;
}

bool AutoCorr::Initialize()
{
	return true;
}

bool AutoCorr::Finalize()
{
	return true;
}

bool AutoCorr::UpdateDynamicParameters()
{
	return true;
}

bool AutoCorr::Run()
{
	const int N = CorrelationLength;
	const int numLags = m_numLags;

	if (N <= 0 || numLags <= 0) {
		POST_ERROR("AutoCorr: invalid internal state (N <= 0 or numLags <= 0).");
		return false;
	}

	for (int n = 0; n < N; ++n) {
		m_samples[static_cast<size_t>(n)] = input[static_cast<size_t>(n)];
	}

	for (int idx = 0; idx < numLags; ++idx) {
		const int lag = StartLag + idx;

		double r = 0.0;
		if (m_CorrelationType == Circular) {
			r = circularAutoCorrelation(lag);
		}
		else {
			r = nonCircularAutoCorrelation(lag);
		}

		switch (m_Normalization) {
		case None:
			break;

		case UnBiased:
			if (m_CorrelationType == NonCircular) {
				{
					const int denom = N - std::abs(lag);
					if (denom > 0) {
						r /= static_cast<double>(denom);
					}
					else {
						r = 0.0;
					}
				}
			}
			else {
				r /= static_cast<double>(N);
			}
			break;

		case Biased:
			r /= static_cast<double>(N);
			break;
		}

		output[static_cast<size_t>(idx)] = r;
	}

	return true;
}

double AutoCorr::nonCircularAutoCorrelation(int lag)
{
	const int N = CorrelationLength;
	double sum = 0.0;

	for (int i = 0; i < N; ++i) {
		const int j = i + lag;
		if (0 <= j && j < N) {
			sum += m_samples[static_cast<size_t>(i)] *
				m_samples[static_cast<size_t>(j)];
		}
	}

	return sum;
}

double AutoCorr::circularAutoCorrelation(int lag)
{
	const int N = CorrelationLength;
	if (N <= 0) {
		return 0.0;
	}

	int k = lag % N;
	if (k < 0) {
		k += N;
	}

	double sum = 0.0;
	for (int i = 0; i < N; ++i) {
		int j = i + k;
		if (j >= N) {
			j -= N;  
		}

		sum += m_samples[static_cast<size_t>(i)] *
			m_samples[static_cast<size_t>(j)];
	}

	return sum;
}
