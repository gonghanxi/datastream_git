// CrossCorr.cpp

#include "CrossCorr.h"
#include <vector>
#include <cmath>

using namespace SystemVueModelBuilder;

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(CrossCorr)
{
	using SystemVueModelBuilder::DFParam;
	using SystemVueModelBuilder::DFPort;

	SET_MODEL_DESCRIPTION("Cross Correlation Estimator");
	SET_MODEL_SYMBOL("SYM_CrossCorr");
	SET_MODEL_CATEGORY("Signal Processing");

	{
		DFPort p = ADD_MODEL_INPUT(input);
		p.SetDescription("input signal x");
	}

	{
		DFPort p = ADD_MODEL_INPUT(input2);
		p.SetDescription("second input signal y");
	}

	{
		DFPort p = ADD_MODEL_OUTPUT(output);
		p.SetDescription("cross-correlation output");
	}

	{
		DFPort p = ADD_MODEL_OUTPUT(delay);
		p.SetDescription("delay of input2 with respect to input (samples)");
	}

	{
		DFParam p = ADD_MODEL_ENUM_PARAM(CorrelationType, CorrelationTypeEnum);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.AddEnumeration("NonCircular", NonCircular);
		p.AddEnumeration("Circular", Circular);
		p.SetDefaultValue("NonCircular");
		p.SetDescription("Correlation method: NonCircular, Circular");
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
		DFParam p = ADD_MODEL_ENUM_PARAM(Normalization, NormalizationEnum);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.AddEnumeration("None", None);
		p.AddEnumeration("UnBiased", UnBiased);
		p.AddEnumeration("Biased", Biased);
		p.SetDefaultValue("None");
		p.SetDescription("Correlation estimate normalization: None, UnBiased, Biased");
	}

	return true;
}
#endif 

CrossCorr::CrossCorr()
	: CorrelationType(NonCircular),   
	CorrelationLength(500),
	StartLag(-50),
	StopLag(50),
	Normalization(None)
{
}

bool CrossCorr::Initialize()
{
	return true;
}

bool CrossCorr::Finalize()
{
	return true;
}

bool CrossCorr::UpdateDynamicParameters()
{
	return true;
}

bool CrossCorr::Setup()
{
	const int N = CorrelationLength;

	if (N <= 0)
	{
		POST_ERROR("CorrelationLength must be > 0.");
        LOG_ERROR("CorrelationLength must be > 0.");
		return false;
	}

	if (StartLag > StopLag)
	{
		POST_ERROR("StartLag must be <= StopLag.");
        LOG_ERROR("CorrelationLength must be > 0.");
		return false;
	}

	const int numLags = StopLag - StartLag + 1;
	if (numLags <= 0)
	{
		POST_ERROR("StopLag - StartLag + 1 must be > 0.");
        LOG_ERROR("CorrelationLength must be > 0.");
		return false;
	}

	// 为避免访问越界：-N < k < N
	if (StartLag <= -N || StopLag >= N)
	{
		POST_ERROR("StartLag and StopLag must satisfy -CorrelationLength < StartLag and StopLag < CorrelationLength.");
        LOG_ERROR("CorrelationLength must be > 0.");
		return false;
	}

	const unsigned uN = static_cast<unsigned>(N);
	const unsigned uLags = static_cast<unsigned>(numLags);

	input.SetRate(uN);
	input2.SetRate(uN);
	output.SetRate(uLags);
	delay.SetRate(1U);

	return true;
}

bool CrossCorr::Run()
{
	const int N = CorrelationLength;
	const int kStart = StartLag;
	const int kStop = StopLag;
	const int numLags = kStop - kStart + 1;

	if (N <= 0 || numLags <= 0)
	{
		POST_ERROR("Invalid parameters in Run().");
		return false;
	}

	std::vector<double> x(static_cast<std::size_t>(N));
	std::vector<double> y(static_cast<std::size_t>(N));

	for (int n = 0; n < N; ++n)
	{
		x[static_cast<std::size_t>(n)] = input[static_cast<std::size_t>(n)];
		y[static_cast<std::size_t>(n)] = input2[static_cast<std::size_t>(n)];
	}

	double bestMetric = -1.0;  
	int    bestLag = 0;

	for (int k = kStart; k <= kStop; ++k)
	{
		double sum = 0.0;

		if (CorrelationType == NonCircular)
		{
			int iStart = (k >= 0) ? 0 : -k;
			int iEnd = (k >= 0) ? N - k : N;   

			for (int i = iStart; i < iEnd; ++i)
			{
				sum += x[static_cast<std::size_t>(i)]
					* y[static_cast<std::size_t>(i + k)];
			}

			if (Normalization == UnBiased)
			{
				const int denom = N - std::abs(k);
				if (denom > 0)
					sum /= static_cast<double>(denom);
			}
			else if (Normalization == Biased)
			{
				sum /= static_cast<double>(N);
			}
		}
		else 
		{
			for (int i = 0; i < N; ++i)
			{
				int j = i + k;
				j = ((j % N) + N) % N;

				sum += x[static_cast<std::size_t>(i)]
					* y[static_cast<std::size_t>(j)];
			}

			if (Normalization == UnBiased || Normalization == Biased)
			{
				sum /= static_cast<double>(N);
			}
		}

		const int outIdx = k - kStart;
		output[static_cast<std::size_t>(outIdx)] = sum;

		const double metric = std::fabs(sum);
		if (metric > bestMetric)
		{
			bestMetric = metric;
			bestLag = k;
		}
	}

	delay[0U] = bestLag;

	return true;
}
