#include "AddGuard.h"

#include <algorithm>
#include <stdexcept>

using namespace SystemVueModelBuilder;

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(AddGuard)
{
	SET_MODEL_DESCRIPTION("OFDM Symbol Guard Samples Inserter");
	SET_MODEL_SYMBOL("SYM_AddGuard");
	SET_MODEL_CATEGORY("Communications");

	{
		auto p = ADD_MODEL_INPUT(m_cbInput);
		p.SetName("In");
		p.SetDescription("Transmitted signal after IFFT");
	}
	{
		auto p = ADD_MODEL_INPUT(m_cbWindow);
		p.SetName("Window");
		p.SetDescription("Window function");
	}
	{
		auto p = ADD_MODEL_OUTPUT(m_cbOutput);
		p.SetName("Out");
		p.SetDescription("OFDM output data");
	}

	{
		auto p = ADD_MODEL_PARAM(m_iIFFTSize);
		p.SetName("IFFTSize");
		p.SetDefaultValue("64");
		p.SetDescription("IFFT size");
	}
	{
		auto p = ADD_MODEL_PARAM(m_iPreGuard);
		p.SetName("PreGuard");
		p.SetDefaultValue("16");
		p.SetDescription("Pre-guard length");
	}
	{
		auto p = ADD_MODEL_PARAM(m_iPostGuard);
		p.SetName("PostGuard");
		p.SetDefaultValue("0");
		p.SetDescription("Post-guard length");
	}
	{
		auto p = ADD_MODEL_PARAM(m_iIntersection);
		p.SetName("Intersection");
		p.SetDefaultValue("0");
		p.SetDescription("Guard intersection length");
	}

	return true;
}
#endif 

AddGuard::AddGuard()
	: m_cbInput()
	, m_cbOutput()
	, m_cbWindow()
	, m_iIFFTSize(64)
	, m_iPreGuard(16)
	, m_iPostGuard(0)
	, m_iIntersection(0)
	, m_iNout(0)
	, m_iNwin(0)
	, m_iNperiod(0)
	, m_cplxBuffer(nullptr)
{
}

AddGuard::~AddGuard()
{
	if (m_cplxBuffer)
	{
		delete[] m_cplxBuffer;
		m_cplxBuffer = nullptr;
	}
}

void AddGuard::ClearCplxBuffer()
{
	if (m_cplxBuffer && m_iIntersection > 0)
	{
		for (int i = 0; i < m_iIntersection; ++i)
			m_cplxBuffer[i] = std::complex<double>(0.0, 0.0);
	}
}

bool AddGuard::Setup()
{
	if (m_iIFFTSize < 1)
		m_iIFFTSize = 1;

	if (m_iPreGuard < 0)
		m_iPreGuard = 0;
	if (m_iPostGuard < 0)
		m_iPostGuard = 0;

	if (m_iPreGuard > m_iIFFTSize)
		m_iPreGuard = m_iIFFTSize;
	if (m_iPostGuard > m_iIFFTSize)
		m_iPostGuard = m_iIFFTSize;

	if (m_iIntersection < 0)
		m_iIntersection = 0;

	const int guardSum = m_iPreGuard + m_iPostGuard;
	if (m_iIntersection > guardSum)
		m_iIntersection = guardSum; 

	const int L = m_iPreGuard + m_iIFFTSize + m_iPostGuard;

	if (2 * m_iIntersection > L)
		m_iIntersection = L / 2;

	m_iNwin = static_cast<size_t>(L);                       
	m_iNout = static_cast<size_t>(L - m_iIntersection);     
	m_iNperiod = static_cast<size_t>(m_iIFFTSize + m_iPreGuard);

	m_cbInput.SetRate(static_cast<unsigned>(m_iIFFTSize));
	m_cbWindow.SetRate(static_cast<unsigned>(m_iNwin));
	m_cbOutput.SetRate(static_cast<unsigned>(m_iNout));

	return true;
}

bool AddGuard::Initialize()
{
	if (m_cplxBuffer)
	{
		delete[] m_cplxBuffer;
		m_cplxBuffer = nullptr;
	}

	if (m_iIntersection > 0)
		m_cplxBuffer = new std::complex<double>[m_iIntersection];

	ClearCplxBuffer();
	return true;
}

bool AddGuard::Finalize()
{
	if (m_cplxBuffer)
	{
		delete[] m_cplxBuffer;
		m_cplxBuffer = nullptr;
	}
	return true;
}

bool AddGuard::Run()
{
	const int N = m_iIFFTSize;
	const int Pg = m_iPreGuard;
	const int Po = m_iPostGuard;
	const int I = m_iIntersection;

	const int L = static_cast<int>(m_iNwin);  
	const int Lout = static_cast<int>(m_iNout);  

	auto compute_u = [&](int idx) -> std::complex<double>
	{
		std::complex<double> x;

		if (idx < Pg)
		{
			const int src = N - Pg + idx; 
			x = m_cbInput[src];
		}
		else if (idx < Pg + N)
		{
			const int src = idx - Pg;     
			x = m_cbInput[src];
		}
		else
		{
			const int src = idx - (Pg + N); 
			x = m_cbInput[src];
		}

		const double w = m_cbWindow[idx];
		return x * w;
	};

	if (I <= 0)
	{
		for (int n = 0; n < L; ++n)
			m_cbOutput[n] = compute_u(n);
		return true;
	}

	int outIdx = 0;

	for (int n = 0; n < I; ++n)
	{
		const std::complex<double> u = compute_u(n);
		const std::complex<double> prev =
			(m_cplxBuffer && n < m_iIntersection)
			? m_cplxBuffer[n]
			: std::complex<double>(0.0, 0.0);

		m_cbOutput[outIdx++] = prev + u;
	}

	const int middleStart = I;
	const int middleEnd = L - I; // [I, L-I)
	for (int n = middleStart; n < middleEnd; ++n)
	{
		m_cbOutput[outIdx++] = compute_u(n);
	}

	if (m_cplxBuffer)
	{
		for (int k = 0; k < I; ++k)
		{
			const int n = L - I + k;        
			m_cplxBuffer[k] = compute_u(n);
		}
	}

	(void)Lout; 

	return true;
}
