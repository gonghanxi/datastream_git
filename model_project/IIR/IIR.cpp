#include "IIR.h"

using namespace SystemVueModelBuilder;


#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(IIR)
{
	ADD_MODEL_HEADER_FILE("IIR.h");
	SET_MODEL_NAMESPACE("SystemVueModelBuilder");

	SET_MODEL_DESCRIPTION("IIR Filter");
	SET_MODEL_SYMBOL("SYM_IIR");
	SET_MODEL_CATEGORY("Filters");

	DFPort port = ADD_MODEL_INPUT(m_input);
	port.SetName("input");
	port.SetDescription("input (real)");

	port = ADD_MODEL_OUTPUT(m_output);
	port.SetName("output");
	port.SetDescription("output (real)");

	DFParam param = ADD_MODEL_PARAM(m_Gain);
	param.SetName("Gain");
	param.SetDescription("Gain Value");
	param.SetDefaultValue("1");

	param = ADD_MODEL_ARRAY_PARAM(m_Numerator, m_iNumeratorSize);
	param.SetName("Numerator");
	param.SetDescription("Numerator coefficients");
	param.SetDefaultValue("[0.5, 0.25, 0.1]");

	param = ADD_MODEL_ARRAY_PARAM(m_Denominator, m_iDenominatorSize);
	param.SetName("Denominator");
	param.SetDescription("Denominator coefficients");
	param.SetDefaultValue("[1, 0.5, 0.3]");

	return true;
}
#endif 

IIR::IIR()
	: m_Gain(1.0)
	, m_Numerator(nullptr)
	, m_Denominator(nullptr)
	, m_iNumeratorSize(0)
	, m_iDenominatorSize(0)
	, m_iNumState(0)
	, m_State(nullptr)
{
}

IIR::~IIR()
{
	if (m_State)
	{
		delete[] m_State;
		m_State = nullptr;
	}
}

bool IIR::Initialize()
{
	if (!m_Denominator || m_iDenominatorSize <= 0)
	{
		POST_ERROR("IIR: Denominator coefficients are not specified.");
		return false;
	}

	double a0 = m_Denominator[0];
	if (std::fabs(a0) < DBL_EPSILON)
	{
		POST_ERROR("IIR: Denominator[0] must be non-zero.");
		return false;
	}

	if (!m_Numerator || m_iNumeratorSize <= 0)
	{
		POST_WARNING("IIR: Numerator is empty, filter output will be zero.");
	}

	const int Nb = m_iNumeratorSize;
	const int Na = m_iDenominatorSize;

	m_iNumState = (Nb > Na ? Nb : Na) - 1;
	if (m_iNumState < 0)
		m_iNumState = 0;

	if (m_State)
	{
		delete[] m_State;
		m_State = nullptr;
	}

	if (m_iNumState > 0)
	{
		m_State = new double[static_cast<size_t>(m_iNumState)];
		for (int i = 0; i < m_iNumState; ++i)
			m_State[i] = 0.0;
	}

	return true;
}

bool IIR::Run()
{
	const int Nb = m_iNumeratorSize;
	const int Na = m_iDenominatorSize;

	if (!m_Denominator || Na <= 0)
	{
		m_output[0] = 0.0;
		return false;
	}

	const double a0 = m_Denominator[0];
	if (std::fabs(a0) < DBL_EPSILON)
	{
		m_output[0] = 0.0;
		return false;
	}

	const double x = m_input[0];
	double y = 0.0;

	if (m_iNumState == 0)
	{
		double acc = 0.0;
		if (Nb > 0)
			acc = m_Numerator[0] * x;
		y = (m_Gain * acc) / a0;
	}
	else
	{
		double* s = m_State;
		const int N = m_iNumState;

		double acc = (Nb > 0 ? m_Numerator[0] * x : 0.0) + s[0];
		y = (m_Gain * acc) / a0;

		for (int i = 0; i < N - 1; ++i)
		{
			double next = s[i + 1];

			if (i + 1 < Nb)
				next += m_Numerator[i + 1] * x;

			if (i + 1 < Na)
				next -= m_Denominator[i + 1] * y;

			s[i] = next;
		}

		double last = 0.0;
		if (N < Nb)
			last += m_Numerator[N] * x;
		if (N < Na)
			last -= m_Denominator[N] * y;

		s[N - 1] = last;
	}

	m_output[0] = y;
	return true;
}

bool IIR::Finalize()
{
	if (m_State)
	{
		delete[] m_State;
		m_State = nullptr;
	}
	m_iNumState = 0;
	return true;
}
