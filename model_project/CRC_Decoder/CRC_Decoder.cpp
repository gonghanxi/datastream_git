#include "CRC_Decoder.h"

#include <algorithm>

using namespace SystemVueModelBuilder;

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(CRC_Decoder)
{
	SET_MODEL_DESCRIPTION("CRC Decoder");
	SET_MODEL_SYMBOL("SYM_CRC_Decoder");
	SET_MODEL_CATEGORY("Communications");

	{
		auto p = ADD_MODEL_INPUT(In);
		p.SetName("In");
		p.SetDescription("Input bits");
	}
	{
		auto p = ADD_MODEL_OUTPUT(Out);
		p.SetName("Out");
		p.SetDescription("Output message bits");
	}
	{
		auto p = ADD_MODEL_OUTPUT(Parity);
		p.SetName("Parity");
		p.SetDescription("Parity check");
	}

	{
		auto p = ADD_MODEL_ENUM_PARAM(ParityPosition, CRC_Decoder::ParityPositionEnum);
		p.SetName("ParityPosition");
		p.AddEnumeration("Tail", CRC_Decoder::Tail);
		p.AddEnumeration("Head", CRC_Decoder::Head);
		p.SetDefaultValue("Tail");
		p.SetDescription("Parity bits position");
	}
	{
		auto p = ADD_MODEL_ENUM_PARAM(ReverseData, CRC_Decoder::YesNoEnum);
		p.SetName("ReverseData");
		p.AddEnumeration("NO", CRC_Decoder::NO);
		p.AddEnumeration("YES", CRC_Decoder::YES);
		p.SetDefaultValue("NO");
		p.SetDescription("Reverse the data sequence");
	}
	{
		auto p = ADD_MODEL_ENUM_PARAM(ReverseParity, CRC_Decoder::YesNoEnum);
		p.SetName("ReverseParity");
		p.AddEnumeration("NO", CRC_Decoder::NO);
		p.AddEnumeration("YES", CRC_Decoder::YES);
		p.SetDefaultValue("NO");
		p.SetDescription("Peverse the parity bits");
	}
	{
		auto p = ADD_MODEL_ENUM_PARAM(ComplementParity, CRC_Decoder::YesNoEnum);
		p.SetName("ComplementParity");
		p.AddEnumeration("NO", CRC_Decoder::NO);
		p.AddEnumeration("YES", CRC_Decoder::YES);
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

CRC_Decoder::CRC_Decoder()
	: In()
	, Out()
	, Parity()
	, ParityPosition(Tail)
	, ReverseData(NO)
	, ReverseParity(NO)
	, ComplementParity(NO)
	, MessageLength(172)
	, InitialState(0)
	, Polynomial(7955)
	, m_InputFrmLen(0)
	, m_CRCLength(0)
	, m_crcMask(0)
	, m_polyNoMsb(0)
	, m_msgFrame(nullptr)
	, m_msgLogical(nullptr)
	, m_crcRx(nullptr)
	, m_crcExp(nullptr)
{
}

CRC_Decoder::~CRC_Decoder()
{
	delete[] m_msgFrame;
	delete[] m_msgLogical;
	delete[] m_crcRx;
	delete[] m_crcExp;

	m_msgFrame = nullptr;
	m_msgLogical = nullptr;
	m_crcRx = nullptr;
	m_crcExp = nullptr;
}

int CRC_Decoder::computeCRCLength(int poly) const
{
	if (poly <= 0) return -1;
	int r = 0;
	int p = poly;
	while (p >>= 1) ++r; 
	return r;
}

void CRC_Decoder::computePolynomialMasks()
{
	m_CRCLength = computeCRCLength(Polynomial);

	if (m_CRCLength <= 0 || m_CRCLength >= 31)
	{
		m_crcMask = 0;
		m_polyNoMsb = 0;
		return;
	}

	m_crcMask = (1u << (uint32_t)m_CRCLength) - 1u;
	m_polyNoMsb = ((uint32_t)Polynomial) & m_crcMask; 
}

int CRC_Decoder::boundaryCheck(char /*functionTag*/)
{
	if (MessageLength <= 0) return -1;
	if (Polynomial <= 0)    return -2;

	computePolynomialMasks();
	if (m_CRCLength <= 0)   return -3;

	return 0;
}

void CRC_Decoder::crcComputeRemainderBits(const bool* msgLogical, bool* crcBits)
{
	const int r = m_CRCLength;
	const uint32_t mask = m_crcMask;
	const uint32_t poly = m_polyNoMsb;

	uint32_t reg = ((uint32_t)InitialState) & mask;

	auto update_with_bit = [&](int inBit)
	{
		inBit = (inBit != 0) ? 1 : 0;

		const int msb = (int)((reg >> (r - 1)) & 1u);
		const int fb = msb ^ inBit;

		reg = ((reg << 1) & mask);
		if (fb) reg ^= poly;
	};

	for (int i = 0; i < MessageLength; ++i)
		update_with_bit(msgLogical[i] ? 1 : 0);

	for (int i = 0; i < r; ++i)
		crcBits[i] = (((reg >> (r - 1 - i)) & 1u) != 0);
}

bool CRC_Decoder::Setup()
{
	const int chk = boundaryCheck('S');
	if (chk != 0)
	{
		if (chk == -1) POST_ERROR("MessageLength must be > 0.");
		if (chk == -2) POST_ERROR("Polynomial must be > 0.");
		if (chk == -3) POST_ERROR("Invalid Polynomial: cannot determine CRCLength.");
		return false;
	}

	m_InputFrmLen = MessageLength + m_CRCLength;

	In.SetRate((unsigned)m_InputFrmLen);
	Out.SetRate((unsigned)MessageLength);
	Parity.SetRate(1u);

	return true;
}

bool CRC_Decoder::Initialize()
{
	delete[] m_msgFrame;
	delete[] m_msgLogical;
	delete[] m_crcRx;
	delete[] m_crcExp;

	m_msgFrame = new bool[MessageLength];
	m_msgLogical = new bool[MessageLength];
	m_crcRx = new bool[m_CRCLength];
	m_crcExp = new bool[m_CRCLength];

	std::fill(m_msgFrame, m_msgFrame + MessageLength, false);
	std::fill(m_msgLogical, m_msgLogical + MessageLength, false);
	std::fill(m_crcRx, m_crcRx + m_CRCLength, false);
	std::fill(m_crcExp, m_crcExp + m_CRCLength, false);

	return true;
}

bool CRC_Decoder::Finalize()
{
	return true;
}

bool CRC_Decoder::UpdateDynamicParameters()
{
	const int oldMsg = (m_msgFrame ? MessageLength : -1);
	const int oldCrc = m_CRCLength;

	if (!Setup())
		return false;

	if (MessageLength != oldMsg || m_CRCLength != oldCrc || !m_msgFrame)
		return Initialize();

	return true;
}

bool CRC_Decoder::Run()
{
	if (ParityPosition == Tail)
	{
		for (int i = 0; i < MessageLength; ++i)
			m_msgFrame[i] = (In[i] != 0);

		for (int i = 0; i < m_CRCLength; ++i)
			m_crcRx[i] = (In[MessageLength + i] != 0);
	}
	else // Head
	{
		for (int i = 0; i < m_CRCLength; ++i)
			m_crcRx[i] = (In[i] != 0);

		for (int i = 0; i < MessageLength; ++i)
			m_msgFrame[i] = (In[m_CRCLength + i] != 0);
	}

	for (int i = 0; i < MessageLength; ++i)
		m_msgLogical[i] = m_msgFrame[i];

	if (ReverseData == YES)
		std::reverse(m_msgLogical, m_msgLogical + MessageLength);

	crcComputeRemainderBits(m_msgLogical, m_crcExp);

	if (ReverseParity == YES)
		std::reverse(m_crcExp, m_crcExp + m_CRCLength);

	if (ComplementParity == YES)
	{
		for (int i = 0; i < m_CRCLength; ++i)
			m_crcExp[i] = !m_crcExp[i];
	}

	bool pass = true;
	for (int i = 0; i < m_CRCLength; ++i)
	{
		if (m_crcExp[i] != m_crcRx[i])
		{
			pass = false;
			break;
		}
	}

	for (int i = 0; i < MessageLength; ++i)
		Out[i] = m_msgLogical[i];

	Parity[0] = pass ? 0 : 1;

	return true;
}
