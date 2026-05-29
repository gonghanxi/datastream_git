#include "AvgSqrErr_M.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(AvgSqrErr_M)
{
	SET_MODEL_DESCRIPTION("Mean Squared Error Matrix Averager (sliding)");
	SET_MODEL_SYMBOL("SYM_AvgSqrErr_M");
	SET_MODEL_CATEGORY("Math Matrix");

	ADD_MODEL_INPUT(input1);
	ADD_MODEL_INPUT(input2);
	ADD_MODEL_OUTPUT(output);

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(NumInputsToAverage);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("8");
		p.SetDescription("Sliding window length (number of matrix pairs)");
	}

	return true;
}
#endif

AvgSqrErr_M::AvgSqrErr_M()
	: NumInputsToAverage(8),
	m_ring(),
	m_head(0),
	m_accumSSE(0.0),
	m_count(0),
	m_rows(0),
	m_cols(0),
	m_shapeInit(false)
{}

bool AvgSqrErr_M::Setup()
{
	input1.SetRate(1U);
	input2.SetRate(1U);
	output.SetRate(1U);

	if (NumInputsToAverage < 1) {
		POST_ERROR("AvgSqrErr_M: NumInputsToAverage must be >= 1.");
		return false;
	}

	m_ring.assign(NumInputsToAverage, 0.0);
	m_head = 0;
	m_accumSSE = 0.0;
	m_count = 0;

	m_rows = 0;
	m_cols = 0;
	m_shapeInit = false;

	return true;
}

bool AvgSqrErr_M::Run()
{
	const SystemVueModelBuilder::Matrix<double>& A = input1[0U];
	const SystemVueModelBuilder::Matrix<double>& B = input2[0U];

	if (!m_shapeInit) {
		m_rows = static_cast<int>(A.NumRows());
		m_cols = static_cast<int>(A.NumColumns());
		m_shapeInit = true;
	}

	if (A.NumRows() != static_cast<std::size_t>(m_rows) ||
		A.NumColumns() != static_cast<std::size_t>(m_cols) ||
		B.NumRows() != static_cast<std::size_t>(m_rows) ||
		B.NumColumns() != static_cast<std::size_t>(m_cols)) {

		POST_ERROR("AvgSqrErr_M: input1 and input2 must keep identical sizes at every call.");
		output[0U] = 0.0;   
		return false;
	}

	const std::size_t N = A.NumElements();   
	double sse = 0.0;
	for (std::size_t i = 0; i < N; ++i) {
		const double d = A(i) - B(i);
		sse += d * d;
	}

	const bool window_full = (m_count >= NumInputsToAverage);
	const double oldest = window_full ? m_ring[m_head] : 0.0;

	if (!window_full) {
		++m_count; 
	}
	m_ring[m_head] = sse;
	m_head = (m_head + 1) % NumInputsToAverage;

	m_accumSSE += sse - oldest;

	const double avg = m_accumSSE / static_cast<double>(m_count);
	output[0U] = avg;

	return true;
}
