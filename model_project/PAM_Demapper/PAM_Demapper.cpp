#include "PAM_Demapper.h"

#include <cmath>
#include <algorithm>

using namespace SystemVueModelBuilder;

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(PAM_Demapper)
{
	SET_MODEL_DESCRIPTION("PAM Demapper");
	SET_MODEL_SYMBOL("SYM_PAM_Demapper");
	SET_MODEL_CATEGORY("Communications");

	{
		DFPort p = ADD_MODEL_INPUT(input);
		p.SetName("Input");
		p.SetDescription("input signal");
	}
	{
		DFPort p = ADD_MODEL_OUTPUT(Bits);
		p.SetName("Bits");
		p.SetDescription("output bit sequence");
	}
	{
		DFPort p = ADD_MODEL_OUTPUT(Amplitude);
		p.SetName("Amplitude");
		p.SetDescription("closest PAM symbol");
	}

	{
		DFParam p = ADD_MODEL_PARAM(NumBits);
		p.SetName("NumBits");
		p.SetDefaultValue("4");
		p.SetDescription("Number of bits");
	}
	{
		DFParam p = ADD_MODEL_ENUM_PARAM(BitOrder, PAM_Demapper::BitOrderE);
		p.SetName("BitOrder");
		p.AddEnumeration("LSB first", PAM_Demapper::LSBFirst);
		p.AddEnumeration("MSB first", PAM_Demapper::MSBFirst);
		p.SetDefaultValue("MSB first");
		p.SetDescription("Bit Order");
	}
	{
		DFParam p = ADD_MODEL_PARAM(HighLevel);
		p.SetName("HighLevel");
		p.SetDefaultValue("1");
		p.SetDescription("Highest level");
	}
	{
		DFParam p = ADD_MODEL_PARAM(LowLevel);
		p.SetName("LowLevel");
		p.SetDefaultValue("-1");
		p.SetDescription("Lowest level");
	}

	return true;
}
#endif 

PAM_Demapper::PAM_Demapper()
	: NumBits(4)
	, BitOrder(MSBFirst)
	, HighLevel(1.0)
	, LowLevel(-1.0)
	, m_levels(16)
	, m_step(0.0)
{
}

void PAM_Demapper::update_cache()
{
	if (NumBits < 1)  NumBits = 1;
	if (NumBits > 30) NumBits = 30;

	m_levels = (1 << NumBits);
	if (m_levels <= 1)
	{
		m_levels = 2;
		m_step = 0.0;
		return;
	}

	const double denom = double(m_levels - 1);
	m_step = (HighLevel - LowLevel) / denom;
}

bool PAM_Demapper::Initialize()
{
	if (NumBits < 1)
	{
		POST_ERROR("PAM_Demapper: NumBits must be >= 1");
		NumBits = 1;
	}
	update_cache();
	return true;
}

bool PAM_Demapper::Setup()
{
	update_cache();

	input.SetRate(1);
	Bits.SetRate(NumBits);
	Amplitude.SetRate(1);

	return true;
}

bool PAM_Demapper::Finalize()
{
	return true;
}

bool PAM_Demapper::Run()
{
	const double x = input[0];

	int idx = 0;

	if (m_levels <= 1 || m_step == 0.0)
	{
		idx = 0;
	}
	else
	{
		const double t = (x - LowLevel) / m_step;
		int qi = (int)std::floor(t + 0.5);
		if (qi < 0) qi = 0;
		if (qi > (m_levels - 1)) qi = (m_levels - 1);
		idx = qi;
	}

	double level = LowLevel;
	if (m_levels > 1)
		level = LowLevel + double(idx) * m_step;

	Amplitude[0] = level;

	if (BitOrder == MSBFirst)
	{
		for (int b = 0; b < NumBits; ++b)
		{
			const int shift = (NumBits - 1 - b);
			Bits[b] = (idx >> shift) & 1;
		}
	}
	else 
	{
		for (int b = 0; b < NumBits; ++b)
		{
			Bits[b] = (idx >> b) & 1;
		}
	}

	return true;
}
