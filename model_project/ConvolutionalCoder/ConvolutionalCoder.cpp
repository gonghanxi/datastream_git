#include "ConvolutionalCoder.h"

using namespace SystemVueModelBuilder;

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(ConvolutionalCoder)
{
	SET_MODEL_DESCRIPTION("Convolutional Coder");
	SET_MODEL_SYMBOL("SYM_ConvolutionalCoder");
	SET_MODEL_CATEGORY("Communications");

	{
		auto p = ADD_MODEL_INPUT(m_cbInput);
		p.SetName("In");
		p.SetDescription("Input bits (boolean)");
	}
	{
		auto p = ADD_MODEL_OUTPUT(m_cbOutput);
		p.SetName("Out");
		p.SetDescription("Coded bits (boolean)");
	}

	{
		auto p = ADD_MODEL_ENUM_PARAM(CodingRate, ConvolutionalCoder::CodingRateEnum);
		p.SetName("CodingRate");
		p.AddEnumeration("rate_1_2", ConvolutionalCoder::rate_1_2);
		p.AddEnumeration("rate_1_3", ConvolutionalCoder::rate_1_3);
		p.AddEnumeration("rate_1_4", ConvolutionalCoder::rate_1_4);
		p.AddEnumeration("rate_1_5", ConvolutionalCoder::rate_1_5);
		p.AddEnumeration("rate_1_6", ConvolutionalCoder::rate_1_6);
		p.AddEnumeration("rate_1_7", ConvolutionalCoder::rate_1_7);
		p.AddEnumeration("rate_1_8", ConvolutionalCoder::rate_1_8);
		p.SetDefaultValue("rate_1_2");
		p.SetDescription("Codingrate");
	}

	{
		auto p = ADD_MODEL_PARAM(ConstraintLength);
		p.SetName("ConstraintLength");
		p.SetDefaultValue("7");
		p.SetDescription("Constraint length");
	}

	{
		auto p = ADD_MODEL_ARRAY_PARAM(Polynomial, PolynomialSize);
		p.SetName("Polynomial");
		p.SetDefaultValue("[91, 121]");
		p.SetDescription("Generator polynomial");
	}

	{
		auto p = ADD_MODEL_ENUM_PARAM(ZeroTail, ConvolutionalCoder::ZeroTailEnum);
		p.SetName("ZeroTail");
		p.AddEnumeration("NO", ConvolutionalCoder::NO);
		p.AddEnumeration("YES", ConvolutionalCoder::YES);
		p.SetDefaultValue("NO");
		p.SetDescription("Zero tail used to convert convolutional code to block code");
	}

	{
		auto p = ADD_MODEL_PARAM(BitSequenceLength);
		p.SetName("BitSequenceLength");
		p.SetDefaultValue("88");
		p.SetDescription("Length of bit squence not including tail bits");
		p.SetHideCondition("ZeroTail ~= 1");
	}

	return true;
}
#endif 

ConvolutionalCoder::ConvolutionalCoder()
	: m_cbInput()
	, m_cbOutput()
	, CodingRate(rate_1_2)
	, ConstraintLength(7)
	, Polynomial(nullptr)
	, PolynomialSize(0)
	, ZeroTail(NO)
	, BitSequenceLength(88)
	, m_Counter(0)
	, m_inputFrmLen(1)
	, m_currentState(0)
	, m_convoCodeRateN(2)
	, m_constraintLenK(7)
	, m_regMaskK(0)
	, m_polyMask{ 0 }
{
}

int ConvolutionalCoder::rateToN(CodingRateEnum r)
{
	switch (r)
	{
	case rate_1_2: return 2;
	case rate_1_3: return 3;
	case rate_1_4: return 4;
	case rate_1_5: return 5;
	case rate_1_6: return 6;
	case rate_1_7: return 7;
	case rate_1_8: return 8;
	default:       return 2;
	}
}

int ConvolutionalCoder::parity_u32(uint32_t v)
{
	v ^= v >> 16;
	v ^= v >> 8;
	v ^= v >> 4;
	v &= 0xF;
	return (0x6996u >> v) & 1u;
}

int ConvolutionalCoder::BitReverse(int mask, int constraintLen)
{
	uint32_t x = (uint32_t)mask;
	uint32_t r = 0;
	for (int i = 0; i < constraintLen; ++i)
	{
		r = (r << 1) | (x & 1u);
		x >>= 1;
	}
	return (int)r;
}

int ConvolutionalCoder::BoundaryCheck(char /*functionTag*/)
{
	if (ConstraintLength < 3)  ConstraintLength = 3;
	if (ConstraintLength > 14) ConstraintLength = 14;

	const int n = rateToN(CodingRate);

	if (Polynomial == nullptr || PolynomialSize <= 0)
		return -1;
	if (PolynomialSize < n)
		return -2;

	if (ZeroTail == YES && BitSequenceLength < 1)
		BitSequenceLength = 1;

	const uint32_t K = (uint32_t)ConstraintLength;
	const uint32_t maxMask = ((1u << K) - 1u);
	const uint32_t msb = 1u << (K - 1);

	for (int i = 0; i < n; ++i)
	{
		const uint32_t p = (uint32_t)Polynomial[i];
		if ((p & msb) == 0u)      return -3;
		if ((p & ~maxMask) != 0u) return -4;
	}

	return 0;
}

bool ConvolutionalCoder::Setup()
{
	const int chk = BoundaryCheck('S');
	if (chk != 0)
	{
		if (chk == -1) POST_ERROR("Polynomial is empty.");
		if (chk == -2) POST_ERROR("Polynomial size < n (CodingRate=1/n).");
		if (chk == -3) POST_ERROR("Polynomial has no MSB tap for given ConstraintLength.");
		if (chk == -4) POST_ERROR("Polynomial contains bits beyond ConstraintLength.");
		return false;
	}

	m_constraintLenK = ConstraintLength;
	m_convoCodeRateN = rateToN(CodingRate);
	m_regMaskK = ((uint32_t)1u << (uint32_t)m_constraintLenK) - 1u;

	if (ZeroTail == YES)
	{
		const int tailLen = (m_constraintLenK - 1);
		m_inputFrmLen = BitSequenceLength + tailLen;

		const int outBits = m_convoCodeRateN * m_inputFrmLen;

		m_cbInput.SetRate((unsigned)m_inputFrmLen);
		m_cbOutput.SetRate((unsigned)outBits);
	}
	else
	{
		m_inputFrmLen = 1;
		m_cbInput.SetRate(1u);
		m_cbOutput.SetRate((unsigned)m_convoCodeRateN);
	}

	return true;
}

bool ConvolutionalCoder::Initialize()
{
	m_Counter = 0;
	m_currentState = 0;

	const int n = m_convoCodeRateN;
	for (int i = 0; i < 8; ++i) m_polyMask[i] = 0;

	for (int i = 0; i < n; ++i)
	{
		const int p = Polynomial[i];
		const int pr = BitReverse(p, m_constraintLenK);
		m_polyMask[i] = (uint32_t)pr & m_regMaskK;
	}

	return true;
}

bool ConvolutionalCoder::Finalize()
{
	return true;
}

bool ConvolutionalCoder::Run()
{
	const int K = m_constraintLenK;
	const int n = m_convoCodeRateN;

	const int tailLen = (K - 1);
	const uint32_t memMask = (tailLen > 0) ? ((1u << (uint32_t)tailLen) - 1u) : 0u;

	uint32_t state = (uint32_t)m_currentState & memMask;

	auto encode_one = [&](int u, int outBase)
	{
		u = (u != 0) ? 1 : 0;

		const uint32_t fullReg = ((state << 1) | (uint32_t)u) & m_regMaskK;

		for (int j = 0; j < n; ++j)
		{
			const int y = parity_u32(fullReg & m_polyMask[j]);
			m_cbOutput[outBase + j] = (y != 0);
		}

		state = ((state << 1) | (uint32_t)u) & memMask;
	};

	if (ZeroTail == YES)
	{
		const int Ninfo = BitSequenceLength;

		if (m_inputFrmLen != (Ninfo + tailLen))
		{
			POST_ERROR("ZeroTail=YES: internal frame length mismatch.");
			return false;
		}

		int outIdx = 0;

		for (int i = 0; i < Ninfo; ++i)
		{
			const int u = m_cbInput[i] ? 1 : 0;
			encode_one(u, outIdx);
			outIdx += n;
		}

		for (int t = 0; t < tailLen; ++t)
		{
			const int u_tail = m_cbInput[Ninfo + t] ? 1 : 0;
			if (u_tail != 0)
			{
				POST_ERROR("Tail bits must be '0' and tail bits number must match the constraint length of convolutional code");
				return false;
			}

			encode_one(0, outIdx);
			outIdx += n;
		}

		if (state != 0u)
		{
			POST_ERROR("ZeroTail=YES: encoder state is not zero after tail bits.");
			return false;
		}

		m_currentState = 0;
		m_Counter++;
	}
	else
	{
		const int u = m_cbInput[0] ? 1 : 0;
		encode_one(u, 0);
		m_currentState = (int)(state & memMask);
	}

	return true;
}
