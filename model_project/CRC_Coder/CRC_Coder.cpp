#include "CRC_Coder.h"

#include <algorithm>

using namespace SystemVueModelBuilder;

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(CRC_Coder)
{
	SET_MODEL_DESCRIPTION("CRC Coder");
	SET_MODEL_SYMBOL("SYM_CRC_Coder");
	SET_MODEL_CATEGORY("Communications");

	{
		auto p = ADD_MODEL_INPUT(In);
		p.SetName("In");
		p.SetDescription("Input bits");
	}
	{
		auto p = ADD_MODEL_OUTPUT(Out);
		p.SetName("Out");
		p.SetDescription("Output bits with CRC parity bits");
	}

	{
		auto p = ADD_MODEL_ENUM_PARAM(ParityPosition, CRC_Coder::ParityPositionEnum);
		p.SetName("ParityPosition");
		p.AddEnumeration("Tail", CRC_Coder::Tail);
		p.AddEnumeration("Head", CRC_Coder::Head);
		p.SetDefaultValue("Tail");
		p.SetDescription("Parity bits position");
	}

	{
		auto p = ADD_MODEL_ENUM_PARAM(ReverseData, CRC_Coder::YesNoEnum);
		p.SetName("ReverseData");
		p.AddEnumeration("NO", CRC_Coder::NO);
		p.AddEnumeration("YES", CRC_Coder::YES);
		p.SetDefaultValue("NO");
		p.SetDescription("Reverse the data sequence");
	}

	{
		auto p = ADD_MODEL_ENUM_PARAM(ReverseParity, CRC_Coder::YesNoEnum);
		p.SetName("ReverseParity");
		p.AddEnumeration("NO", CRC_Coder::NO);
		p.AddEnumeration("YES", CRC_Coder::YES);
		p.SetDefaultValue("NO");
		p.SetDescription("Peverse the parity bits");
	}

	{
		auto p = ADD_MODEL_ENUM_PARAM(ComplementParity, CRC_Coder::YesNoEnum);
		p.SetName("ComplementParity");
		p.AddEnumeration("NO", CRC_Coder::NO);
		p.AddEnumeration("YES", CRC_Coder::YES);
		p.SetDefaultValue("NO");
		p.SetDescription("Complement parity bits");
	}

	{
		auto p = ADD_MODEL_PARAM(MessageLength);
		p.SetName("MessageLength");
		p.SetDefaultValue("172");
		p.SetDescription("Input message length");
	}

	{
		auto p = ADD_MODEL_PARAM(InitialState);
		p.SetName("InitialState");
		p.SetDefaultValue("0");
		p.SetDescription("Initial state of encoder");
	}

	{
		auto p = ADD_MODEL_PARAM(Polynomial);
		p.SetName("Polynomial");
		p.SetDefaultValue("7955");
		p.SetDescription("Generator polynomial");
	}

	return true;
}
#endif 


CRC_Coder::CRC_Coder()
	: In()
	, Out()
	, ParityPosition(Tail)
	, ReverseData(NO)
	, ReverseParity(NO)
	, ComplementParity(NO)
	, MessageLength(172)
	, InitialState(0)
	, Polynomial(7955)
	, m_OutFrmLen(0)
	, m_CRCLength(0)
	, m_crcMask(0)
	, m_polyNoMsb(0)
	, m_frameP(nullptr)
	, m_CRC_P(nullptr)
{
}

CRC_Coder::~CRC_Coder()
{
	delete[] m_frameP;
	delete[] m_CRC_P;
	m_frameP = nullptr;
	m_CRC_P = nullptr;
}


int CRC_Coder::computeCRCLength(int poly) const
{
	if (poly <= 0)
		return -1;

	int r = 0;
	int p = poly;
	while (p >>= 1) ++r;
	return r;
}

void CRC_Coder::computePolynomialMasks()
{
	m_CRCLength = computeCRCLength(Polynomial);

	if (m_CRCLength <= 0 || m_CRCLength >= 31)
	{
		m_crcMask = 0;
		m_polyNoMsb = 0;
		return;
	}

	m_crcMask = (1u << (uint32_t)m_CRCLength) - 1u;

	m_polyNoMsb = (uint32_t)Polynomial & m_crcMask;
}

int CRC_Coder::boundaryCheck(char /*functionTag*/)
{
	if (MessageLength <= 0)
		return -1;

	if (Polynomial <= 0)
		return -2;

	computePolynomialMasks();

	if (m_CRCLength <= 0)
		return -3;

	return 0;
}

void CRC_Coder::crcEncodeOneFrame(const bool* dataBits, bool* crcBits)
{
	const int r = m_CRCLength;
	const uint32_t mask = m_crcMask;
	const uint32_t poly = m_polyNoMsb;

	uint32_t reg = (uint32_t)InitialState & mask;

	auto update_with_bit = [&](int inBit)
	{
		inBit = (inBit != 0) ? 1 : 0;

		const int msb = (int)((reg >> (r - 1)) & 1u);
		const int fb = msb ^ inBit;

		reg = ((reg << 1) & mask);
		if (fb)
			reg ^= poly;
	};

	for (int i = 0; i < MessageLength; ++i)
		update_with_bit(dataBits[i] ? 1 : 0);

	for (int i = 0; i < r; ++i)
		crcBits[i] = (((reg >> (r - 1 - i)) & 1u) != 0);
}


bool CRC_Coder::Setup()
{
	const int chk = boundaryCheck('S');
	if (chk != 0)
	{
		if (chk == -1) POST_ERROR("MessageLength must be > 0.");
		if (chk == -2) POST_ERROR("Polynomial must be > 0.");
		if (chk == -3) POST_ERROR("Invalid Polynomial: cannot determine CRCLength.");
		return false;
	}

	m_OutFrmLen = MessageLength + m_CRCLength;

	In.SetRate((unsigned)MessageLength);
	Out.SetRate((unsigned)m_OutFrmLen);

	return true;
}

bool CRC_Coder::Initialize()
{
	delete[] m_frameP;
	delete[] m_CRC_P;
	m_frameP = nullptr;
	m_CRC_P = nullptr;

	m_frameP = new bool[MessageLength];
	m_CRC_P = new bool[m_CRCLength];

	std::fill(m_frameP, m_frameP + MessageLength, false);
	std::fill(m_CRC_P, m_CRC_P + m_CRCLength, false);

	return true;
}

bool CRC_Coder::Finalize()
{
	return true;
}

bool CRC_Coder::UpdateDynamicParameters()
{
	return Setup();
}

bool CRC_Coder::Run()
{
	for (int i = 0; i < MessageLength; ++i)
		m_frameP[i] = (In[i] != 0);

	crcEncodeOneFrame(m_frameP, m_CRC_P);

	if (ReverseParity == YES)
		std::reverse(m_CRC_P, m_CRC_P + m_CRCLength);

	if (ComplementParity == YES)
	{
		for (int i = 0; i < m_CRCLength; ++i)
			m_CRC_P[i] = !m_CRC_P[i];
	}

	auto writeData = [&](int &outIdx)
	{
		if (ReverseData == YES)
		{
			for (int i = MessageLength - 1; i >= 0; --i)
				Out[outIdx++] = m_frameP[i];
		}
		else
		{
			for (int i = 0; i < MessageLength; ++i)
				Out[outIdx++] = m_frameP[i];
		}
	};

	int outIdx = 0;

	if (ParityPosition == Head)
	{
		for (int i = 0; i < m_CRCLength; ++i)
			Out[outIdx++] = m_CRC_P[i];

		writeData(outIdx);
	}
	else 
	{
		writeData(outIdx);

		for (int i = 0; i < m_CRCLength; ++i)
			Out[outIdx++] = m_CRC_P[i];
	}

	return true;
}
