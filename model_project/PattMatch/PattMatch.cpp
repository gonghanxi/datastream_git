#include "PattMatch.h"

#include <cmath>
#include <limits>

namespace SystemVueModelBuilder
{

#ifndef SV_CODE_GEN
	DEFINE_MODEL_INTERFACE(PattMatch)
	{
		SET_MODEL_DESCRIPTION("Pattern Cross Correlator using Template");
		SET_MODEL_SYMBOL("SYM_PattMatch");
		SET_MODEL_CATEGORY("Signal Processing");

		{
			auto p = ADD_MODEL_INPUT(m_templ);
			p.SetDescription("Template input");
		}
		{
			auto p = ADD_MODEL_INPUT(m_window);
			p.SetDescription("Window input");
		}

		{
			auto p = ADD_MODEL_OUTPUT(m_index);
			p.SetDescription("Index output");
		}
		{
			auto p = ADD_MODEL_OUTPUT(m_values);
			p.SetDescription("Cross-correlation output");
		}

		{
			DFParam p = ADD_MODEL_PARAM(m_tempSize);
			p.SetUnit(Units::NONE);
			p.SetDefaultValue("32");  
			p.SetDescription("Number of samples in template");
		}

		{
			DFParam p = ADD_MODEL_PARAM(m_winSize);
			p.SetUnit(Units::NONE);
			p.SetDefaultValue("176"); 
			p.SetDescription("Number of samples in search template");
		}

		return true;
	}
#endif 

	PattMatch::PattMatch()
		: m_templ()
		, m_window()
		, m_values()
		, m_index(0)
		, m_tempSize(32)
		, m_winSize(176)
		, m_n(0)
	{
	}

	bool PattMatch::ValidateParameters()
	{
		bool ok = true;

		if (m_tempSize <= 0) {
			POST_ERROR("TempSize must be a positive integer.");
			ok = false;
		}
		if (m_winSize <= 0) {
			POST_ERROR("WinSize must be a positive integer.");
			ok = false;
		}
		if (m_winSize < m_tempSize) {
			POST_ERROR("WinSize must be greater than or equal to TempSize.");
			ok = false;
		}

		return ok;
	}

	bool PattMatch::Setup()
	{
		if (!ValidateParameters())
			return false;

		const unsigned int tempSizeU = static_cast<unsigned int>(m_tempSize);
		const unsigned int winSizeU = static_cast<unsigned int>(m_winSize);
		const unsigned int valuesSizeU = winSizeU - tempSizeU + 1U;

		m_templ.SetRate(tempSizeU);
		m_window.SetRate(winSizeU);
		m_values.SetRate(valuesSizeU);

		return true;
	}

	bool PattMatch::Run()
	{
		const int M = m_tempSize;
		const int N = m_winSize;
		const int numOut = N - M + 1;

		if (M <= 0 || N <= 0 || numOut <= 0) {
			POST_ERROR("Invalid TempSize/WinSize settings in Run().");
			return false;
		}

		double maxCorr = std::numeric_limits<double>::lowest();
		int    bestIndex = 0;

		// n = 0 .. N-M
		for (m_n = 0; m_n < numOut; ++m_n)
		{
			double num = 0.0; 
			double den = 0.0; 

			for (int m = 0; m < M; ++m)
			{
				const double t = m_templ[static_cast<unsigned int>(m)];
				const double w = m_window[static_cast<unsigned int>(m_n + m)];

				num += t * w;
				den += w * w;
			}

			double c = 0.0;
			if (den > 0.0) {
				c = num / den;    
			}
			else {
				c = 0.0;
			}

			m_values[static_cast<unsigned int>(m_n)] = c;

			if (c > maxCorr) {
				maxCorr = c;
				bestIndex = m_n;
			}
		}

		m_index = bestIndex;

		return true;
	}

} 