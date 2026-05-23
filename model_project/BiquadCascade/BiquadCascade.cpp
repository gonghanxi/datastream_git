#include "BiquadCascade.h"

using namespace SystemVueModelBuilder;

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(BiquadCascade)
{
	ADD_MODEL_HEADER_FILE("BiquadCascade.h");
	SET_MODEL_NAMESPACE("SystemVueModelBuilder");

	SET_MODEL_DESCRIPTION("IIR Filter with Cascaded Biquad Sections");
	SET_MODEL_SYMBOL("SYM_Biquad");
	SET_MODEL_CATEGORY("Filters");

	DFPort port = ADD_MODEL_INPUT(m_dInput);
	port.SetName("input");
	port.SetDescription("input (real)");

	port = ADD_MODEL_OUTPUT(m_dOutput);
	port.SetName("output");
	port.SetDescription(
		"The outputs from each of the biquads in the cascade, "
		"starting with the output from the last");

	DFParam param = ADD_MODEL_ARRAY_PARAM(m_pdTaps, m_iTapsSize);
	param.SetName("Taps");
	param.SetDescription("Sets of six biquad coefficients");
	param.SetDefaultValue("[0.067455, 0.135, 0.067455, 1, -1.143, 0.4128]");

	return true;
}
#endif 

BiquadCascade::BiquadCascade()
	: m_pdTaps(nullptr)
	, m_iTapsSize(0)
	, m_pdState1(nullptr)
	, m_pdState2(nullptr)
	, m_pBlocks(nullptr)
	, m_iNumBiquads(0)
{
}

BiquadCascade::~BiquadCascade()
{
	Finalize();
}

bool BiquadCascade::Initialize()
{
	if (m_pdState1)
	{
		delete[] m_pdState1;
		m_pdState1 = nullptr;
	}
	if (m_pdState2)
	{
		delete[] m_pdState2;
		m_pdState2 = nullptr;
	}
	if (m_pBlocks)
	{
		delete[] m_pBlocks;
		m_pBlocks = nullptr;
	}
	m_iNumBiquads = 0;

	if (!m_pdTaps || m_iTapsSize <= 0)
	{
		POST_ERROR("BiquadCascade: Taps must contain at least 6 coefficients.");
		return false;
	}

	if (m_iTapsSize % 6 != 0)
	{
		POST_ERROR("BiquadCascade: Taps length must be a multiple of 6 "
			"(N0, N1, N2, D0, D1, D2 for each section).");
		return false;
	}

	m_iNumBiquads = static_cast<std::size_t>(m_iTapsSize / 6);

	m_pBlocks = new BiquadBlock[m_iNumBiquads];
	m_pdState1 = new double[m_iNumBiquads];
	m_pdState2 = new double[m_iNumBiquads];

	for (std::size_t i = 0; i < m_iNumBiquads; ++i)
	{
		const int base = static_cast<int>(6 * i);

		const double N0 = m_pdTaps[base + 0];
		const double N1 = m_pdTaps[base + 1];
		const double N2 = m_pdTaps[base + 2];
		const double D0 = m_pdTaps[base + 3];
		const double D1 = m_pdTaps[base + 4];
		const double D2 = m_pdTaps[base + 5];

		if (std::fabs(D0) < DBL_EPSILON)
		{
			POST_ERROR("BiquadCascade: D0 (denominator constant term) "
				"must be non-zero in each biquad.");
			return false;
		}

		const double invD0 = 1.0 / D0;

		BiquadBlock& b = m_pBlocks[i];
		b.b0 = N0 * invD0;
		b.b1 = N1 * invD0;
		b.b2 = N2 * invD0;
		b.a1 = D1 * invD0;   
		b.a2 = D2 * invD0;   

		m_pdState1[i] = 0.0;
		m_pdState2[i] = 0.0;
	}

	return true;
}

bool BiquadCascade::Run()
{
	if (m_iNumBiquads == 0 || !m_pBlocks)
	{
		m_dOutput[0][0] = 0.0;
		return false;
	}

	double u = m_dInput[0];

	double* stageOut = new double[m_iNumBiquads];

	for (std::size_t i = 0; i < m_iNumBiquads; ++i)
	{
		BiquadBlock& b = m_pBlocks[i];
		double& z1 = m_pdState1[i];
		double& z2 = m_pdState2[i];

		const double t = b.b0 * u + z1;
		const double y = t;

		const double new_z1 = b.b1 * u - b.a1 * y + z2;
		const double new_z2 = b.b2 * u - b.a2 * y;

		z1 = new_z1;
		z2 = new_z2;

		stageOut[i] = y;

		u = y;
	}

	for (std::size_t j = 0; j < m_iNumBiquads; ++j)
	{
		const std::size_t stageIndex = m_iNumBiquads - 1 - j;

		m_dOutput[static_cast<int>(j)][0] = stageOut[stageIndex];
	}

	delete[] stageOut;
	return true;
}

bool BiquadCascade::Finalize()
{
	if (m_pdState1)
	{
		delete[] m_pdState1;
		m_pdState1 = nullptr;
	}
	if (m_pdState2)
	{
		delete[] m_pdState2;
		m_pdState2 = nullptr;
	}
	if (m_pBlocks)
	{
		delete[] m_pBlocks;
		m_pBlocks = nullptr;
	}
	m_iNumBiquads = 0;
	return true;
}
