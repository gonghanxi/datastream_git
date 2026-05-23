#include "Biquad.h"
#include <cfloat>
#include <cmath>

using namespace SystemVueModelBuilder;

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(Biquad)
{
	ADD_MODEL_HEADER_FILE("Biquad.h");
	SET_MODEL_NAMESPACE("SystemVueModelBuilder");

	SET_MODEL_DESCRIPTION("Biquad IIR Filter");
	SET_MODEL_SYMBOL("SYM_Biquad");
	SET_MODEL_CATEGORY("Filters");

	{
		DFPort port = ADD_MODEL_INPUT(m_dInput);
		port.SetName("input");
		port.SetDescription("input (real)");
	}

	{
		DFPort port = ADD_MODEL_OUTPUT(m_dOutput);
		port.SetName("output");
		port.SetDescription("output (real)");
	}

	{
		DFParam param = ADD_MODEL_PARAM(m_dD1);
		param.SetName("D1");
		param.SetDescription("First-order denominator coefficient");
		param.SetDefaultValue("-1.143");
		param.SetDynamicUpdate(true);  
	}

	{
		DFParam param = ADD_MODEL_PARAM(m_dD2);
		param.SetName("D2");
		param.SetDescription("Second-order denominator coefficient");
		param.SetDefaultValue("0.4128");
		param.SetDynamicUpdate(true);
	}

	{
		DFParam param = ADD_MODEL_PARAM(m_dN0);
		param.SetName("N0");
		param.SetDescription("Zeroth-order numerator coefficient");
		param.SetDefaultValue("0.067455");
		param.SetDynamicUpdate(true);
	}

	{
		DFParam param = ADD_MODEL_PARAM(m_dN1);
		param.SetName("N1");
		param.SetDescription("First-order numerator coefficient");
		param.SetDefaultValue("0.135");
		param.SetDynamicUpdate(true);
	}

	{
		DFParam param = ADD_MODEL_PARAM(m_dN2);
		param.SetName("N2");
		param.SetDescription("Second-order numerator coefficient");
		param.SetDefaultValue("0.067455");
		param.SetDynamicUpdate(true);
	}

	return true;
}
#endif 

Biquad::Biquad()
	: m_dInput()
	, m_dOutput()
	, m_dD1(-1.143)
	, m_dD2(0.4128)
	, m_dN0(0.067455)
	, m_dN1(0.135)
	, m_dN2(0.067455)
	, m_dState1(0.0)
	, m_dState2(0.0)
{
}

bool Biquad::Initialize()
{
	m_dState1 = 0.0;
	m_dState2 = 0.0;

	if (!std::isfinite(m_dD1) || !std::isfinite(m_dD2) ||
		!std::isfinite(m_dN0) || !std::isfinite(m_dN1) || !std::isfinite(m_dN2))
	{
		POST_ERROR("Biquad: coefficients must be finite numbers.");
		return false;
	}

	return true;
}

bool Biquad::Run()
{
	const double x = m_dInput[0];

	const double s1 = m_dState1;
	const double s2 = m_dState2;

	const double y = m_dN0 * x + s1;

	const double newS1 = m_dN1 * x - m_dD1 * y + s2;
	const double newS2 = m_dN2 * x - m_dD2 * y;

	m_dState1 = newS1;
	m_dState2 = newS2;

	m_dOutput[0] = y;
	return true;
}

bool Biquad::Finalize()
{
	m_dState1 = 0.0;
	m_dState2 = 0.0;
	return true;
}
