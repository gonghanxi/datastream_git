#include "Quantizer.h"
#include <algorithm> // std::lower_bound

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(Quantizer)
{
	SET_MODEL_DESCRIPTION("Quantizer using Threshold List");
	SET_MODEL_SYMBOL("SYM_Quantizer");
	SET_MODEL_CATEGORY("Signal Processing");

	ADD_MODEL_INPUT(input);
	ADD_MODEL_OUTPUT(output);
	ADD_MODEL_OUTPUT(stepNumber);

	{
		SystemVueModelBuilder::DFParam p =
			ADD_MODEL_ARRAY_PARAM(Thresholds, ThresholdsSize);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
        p.SetDefaultValue("[0]");
		p.SetDescription("Quantization thresholds (increasing order)");
	}

	{
		SystemVueModelBuilder::DFParam p =
			ADD_MODEL_ARRAY_PARAM(Levels, LevelsSize);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("");
		p.SetDescription("Output levels (if empty use 0, 1, 2, ...)");
	}

	return true;
}
#endif 

Quantizer::Quantizer()
	: Thresholds(nullptr)
	, ThresholdsSize(0U)
	, Levels(nullptr)
	, LevelsSize(0U)
{
}

bool Quantizer::Setup()
{
	m_thresholds.clear();
	m_levels.clear();

	if (Thresholds == nullptr || ThresholdsSize == 0U)
	{
		m_thresholds.push_back(0.0);
	}
	else
	{
		m_thresholds.assign(Thresholds,
			Thresholds + ThresholdsSize);
	}

	const unsigned N = static_cast<unsigned>(m_thresholds.size());
	if (N == 0U)
	{
		POST_ERROR("Thresholds must contain at least one element.");
		return false;
	}

	for (unsigned i = 1; i < N; ++i)
	{
		if (!(m_thresholds[i - 1] < m_thresholds[i]))
		{
			POST_ERROR("Thresholds must be in strictly increasing order.");
			return false;
		}
	}

	if (Levels == nullptr || LevelsSize == 0U)
	{
		m_levels.resize(N + 1U);
		for (unsigned k = 0; k <= N; ++k)
		{
			m_levels[k] = static_cast<double>(k);
		}
	}
	else
	{
		m_levels.assign(Levels, Levels + LevelsSize);

		if (m_levels.size() != N + 1U)
		{
			POST_ERROR("Levels must have exactly N+1 elements.");
			return false;
		}
	}

	input.SetRate(1U);
	output.SetRate(1U);
	stepNumber.SetRate(1U);

	return true;
}

bool Quantizer::Run()
{
	const double x = input[0U];

	const unsigned N =
		static_cast<unsigned>(m_thresholds.size());

	if (N == 0U || m_levels.size() != N + 1U)
	{
		POST_ERROR("Quantizer internal state is invalid.");
		return false;
	}

	const auto it =
		std::lower_bound(m_thresholds.begin(),
			m_thresholds.end(), x);

	unsigned k = 0U;
	if (it == m_thresholds.end())
	{
		k = N;           
	}
	else
	{
		k = static_cast<unsigned>(it - m_thresholds.begin()); // 0..N-1
	}

	output[0U] = m_levels[k];
	stepNumber[0U] = static_cast<int>(k);

	return true;
}
