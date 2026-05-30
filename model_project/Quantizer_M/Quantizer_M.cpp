#include "Quantizer_M.h"
#include <limits>

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(Quantizer_M)
{
	SET_MODEL_DESCRIPTION("Quantizer using Threshold List");
	SET_MODEL_SYMBOL("SYM_Quantizer");
	SET_MODEL_CATEGORY("Math Matrix");

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
		p.SetDescription("Output levels (if empty use 0, 1, 2, ...)");
	}

	return true;
}
#endif 

Quantizer_M::Quantizer_M()
	: Thresholds(nullptr)
	, ThresholdsSize(0)
	, Levels(nullptr)
	, LevelsSize(0)
{}

bool Quantizer_M::Setup()
{
	if (!ValidateParameters())
		return false;

	BuildInternalTables();

	input.SetRate(1U);
	output.SetRate(1U);
	stepNumber.SetRate(1U);

	return true;
}

bool Quantizer_M::ValidateParameters()
{
	const int N = ThresholdsSize;

	if (N <= 0 || Thresholds == nullptr)
	{
		POST_ERROR("Quantizer_M: Thresholds must contain at least one element.");
		return false;
	}

	for (int i = 1; i < N; ++i)
	{
		if (!(Thresholds[i - 1] < Thresholds[i]))
		{
			POST_ERROR("Quantizer_M: Thresholds must be in strictly increasing order.");
			return false;
		}
	}

	if (LevelsSize != 0 && Levels == nullptr)
	{
		POST_ERROR("Quantizer_M: LevelsSize is non-zero but Levels pointer is null.");
		return false;
	}

	if (LevelsSize != 0 && LevelsSize != (N + 1))
	{
		POST_ERROR("Quantizer_M: Levels length must be N+1 where N is the number of thresholds.");
		return false;
	}

	return true;
}

void Quantizer_M::BuildInternalTables()
{
	const int N = ThresholdsSize;

	m_thresholds.assign(Thresholds, Thresholds + N);

	if (LevelsSize == 0 || Levels == nullptr)
	{
		m_levels.resize(N + 1);
		for (int k = 0; k <= N; ++k)
			m_levels[static_cast<std::size_t>(k)] = static_cast<double>(k);
	}
	else
	{
		m_levels.assign(Levels, Levels + LevelsSize);
	}
}

int Quantizer_M::QuantizeIndex(double x) const
{
	const int N = static_cast<int>(m_thresholds.size());

	std::vector<double>::const_iterator it =
		std::lower_bound(m_thresholds.begin(), m_thresholds.end(), x);

	int k = static_cast<int>(it - m_thresholds.begin());
	if (k < 0) k = 0;
	if (k > N) k = N;
	return k;
}

bool Quantizer_M::Run()
{
	SystemVueModelBuilder::DoubleMatrix &inMat = input[0U];
	SystemVueModelBuilder::DoubleMatrix &outMat = output[0U];
	SystemVueModelBuilder::IntMatrix    &idxMat = stepNumber[0U];

	const std::size_t rows = inMat.NumRows();
	const std::size_t cols = inMat.NumColumns();

	outMat.Resize(rows, cols);
	idxMat.Resize(rows, cols);

	for (std::size_t c = 0; c < cols; ++c)
	{
		for (std::size_t r = 0; r < rows; ++r)
		{
			const double x = inMat(r, c);

			const int k = QuantizeIndex(x);

			outMat(r, c) = m_levels[static_cast<std::size_t>(k)];
			idxMat(r, c) = k;
		}
	}

	return true;
}
