#include "GrayDecoder.h"

#include <algorithm> 
#include <cstring>   

using namespace SystemVueModelBuilder;

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(GrayDecoder)
{
	SET_MODEL_DESCRIPTION("Gray Decoder");
	SET_MODEL_SYMBOL("SYM_GrayDecoder");
	SET_MODEL_CATEGORY("Communications");

	{
		DFPort p = ADD_MODEL_INPUT(input);
		p.SetName("input");
		p.SetDescription("input bits");
	}
	{
		DFPort p = ADD_MODEL_OUTPUT(output);
		p.SetName("output");
		p.SetDescription("output bits");
	}

	{
		DFParam p = ADD_MODEL_PARAM(NumBits);
		p.SetName("NumBits");
		p.SetDefaultValue("4");
		p.SetDescription("Number of bits read/produced per execution");
	}
	{
		DFParam p = ADD_MODEL_ENUM_PARAM(m_BitOrder, GrayDecoder::BitOrderE);
		p.SetName("BitOrder");
		p.AddEnumeration("LSB first", GrayDecoder::LSB_first);
		p.AddEnumeration("MSB first", GrayDecoder::MSB_first);
		p.SetDefaultValue("MSB first");
		p.SetDescription("Bit Order");
	}

	return true;
}
#endif 

GrayDecoder::GrayDecoder()
	: NumBits(4)
	, m_BitOrder(MSB_first)
	, inBits(nullptr)
	, outBits(nullptr)
{
}

GrayDecoder::~GrayDecoder()
{
	FreeBuffers();
}

void GrayDecoder::FreeBuffers()
{
	delete[] inBits;  inBits = nullptr;
	delete[] outBits; outBits = nullptr;
}

void GrayDecoder::AllocBuffers(int n)
{
	FreeBuffers();
	if (n <= 0) return;
	inBits = new bool[n];
	outBits = new bool[n];
	std::memset(inBits, 0, sizeof(bool) * n);
	std::memset(outBits, 0, sizeof(bool) * n);
}

bool GrayDecoder::Setup()
{
	int n = (NumBits > 0) ? NumBits : 1;
	input.SetRate(n);
	output.SetRate(n);
	return true;
}

bool GrayDecoder::Initialize()
{
	if (NumBits <= 0)
	{
		POST_ERROR("GrayDecoder: NumBits must be > 0");
		NumBits = 1;
	}
	AllocBuffers(NumBits);
	return true;
}

bool GrayDecoder::Finalize()
{
	return true;
}

bool GrayDecoder::Run()
{
	const int n = (NumBits > 0) ? NumBits : 1;

	if (m_BitOrder == MSB_first)
	{
		for (int k = 0; k < n; ++k)
			inBits[n - 1 - k] = input[k];
	}
	else
	{
		for (int k = 0; k < n; ++k)
			inBits[k] = input[k];
	}

	outBits[n - 1] = inBits[n - 1]; 
	for (int i = n - 2; i >= 0; --i)
		outBits[i] = (outBits[i + 1] ^ inBits[i]);

	if (m_BitOrder == MSB_first)
	{
		for (int k = 0; k < n; ++k)
			output[k] = outBits[n - 1 - k];
	}
	else
	{
		for (int k = 0; k < n; ++k)
			output[k] = outBits[k];
	}

	return true;
}
