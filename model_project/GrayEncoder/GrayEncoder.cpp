#include "GrayEncoder.h"

#include <cstring>   

using namespace SystemVueModelBuilder;

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(GrayEncoder)
{
	SET_MODEL_DESCRIPTION("Gray Encoder");
	SET_MODEL_SYMBOL("SYM_GrayEncoder");
	SET_MODEL_CATEGORY("Communications");

	{
		DFPort p = ADD_MODEL_INPUT(input);
		p.SetName("input");
		p.SetDescription("input");
	}
	{
		DFPort p = ADD_MODEL_OUTPUT(output);
		p.SetName("output");
		p.SetDescription("output");
	}

	{
		DFParam p = ADD_MODEL_PARAM(NumBits);
		p.SetName("NumBits");
		p.SetDefaultValue("4");
		p.SetDescription("Number of bits read/produced per execution");
	}
	{
		DFParam p = ADD_MODEL_ENUM_PARAM(m_BitOrder, GrayEncoder::BitOrder);
		p.SetName("BitOrder");
		p.AddEnumeration("LSB first", GrayEncoder::LSB_first);
		p.AddEnumeration("MSB first", GrayEncoder::MSB_first);
		p.SetDefaultValue("MSB first");
		p.SetDescription("Bit Order");
	}

	return true;
}
#endif 


GrayEncoder::GrayEncoder()
	: NumBits(4)
	, m_BitOrder(MSB_first)
	, m_inBits(nullptr)
	, m_outBits(nullptr)
{
}

GrayEncoder::~GrayEncoder()
{
	FreeBuffers();
}

void GrayEncoder::FreeBuffers()
{
	delete[] m_inBits;
	delete[] m_outBits;
	m_inBits = nullptr;
	m_outBits = nullptr;
}

bool GrayEncoder::EnsureBuffers()
{
	if (NumBits <= 0)
	{
        LOG_ERROR("GrayEncoder: NumBits must be > 0");
		return false;
	}

	FreeBuffers();
	m_inBits = new bool[NumBits];
	m_outBits = new bool[NumBits];

	// Init to 0
	memset(m_inBits, 0, sizeof(bool) * NumBits);
	memset(m_outBits, 0, sizeof(bool) * NumBits);

	return true;
}

bool GrayEncoder::Setup()
{
	if (NumBits <= 0)
	{
		POST_ERROR("GrayEncoder: NumBits must be > 0");
		NumBits = 1;
	}

	input.SetRate(NumBits);
	output.SetRate(NumBits);
	return true;
}

bool GrayEncoder::Initialize()
{
	return EnsureBuffers();
}

bool GrayEncoder::Finalize()
{
	return true;
}

void GrayEncoder::GrayEncodeBitsLSB0(const bool* binLSB0, bool* grayLSB0, int nbits)
{
	if (nbits <= 0) return;

	if (nbits == 1)
	{
		grayLSB0[0] = binLSB0[0];
		return;
	}

	const int msb = nbits - 1;
	grayLSB0[msb] = binLSB0[msb];
	for (int i = 0; i < msb; ++i)
	{
		grayLSB0[i] = (binLSB0[i] != binLSB0[i + 1]); 
	}
}

bool GrayEncoder::Run()
{
	if (NumBits <= 0)
	{
		POST_ERROR("GrayEncoder: NumBits must be > 0");
		return true;
	}

	if (!m_inBits || !m_outBits)
	{
		if (!EnsureBuffers())
			return true;
	}

	if (m_BitOrder == MSB_first)
	{
		for (int k = 0; k < NumBits; ++k)
			m_inBits[NumBits - 1 - k] = input[k];
	}
	else 
	{
		for (int k = 0; k < NumBits; ++k)
			m_inBits[k] = input[k];
	}

	GrayEncodeBitsLSB0(m_inBits, m_outBits, NumBits);

	if (m_BitOrder == MSB_first)
	{
		for (int k = 0; k < NumBits; ++k)
			output[k] = m_outBits[NumBits - 1 - k];
	}
	else 
	{
		for (int k = 0; k < NumBits; ++k)
			output[k] = m_outBits[k];
	}

	return true;
}
