#include "Mapper.h"

#include <cmath>
#include <set>

using namespace SystemVueModelBuilder;

namespace
{
	const double SV_PI = 3.141592653589793238462643383279502884;

	static unsigned int BitReverse2(unsigned int x)
	{
		return ((x & 0x1u) << 1) | ((x >> 1) & 0x1u);
	}

	// 仅用于 1024/4096-QAM：
	// q2解释为 I/Q 的符号位
	static std::complex<double> ApplyQuadrantSigns1024_4096(int q2, double I, double Q)
	{
		switch (q2 & 0x3)
		{
		case 0b00: return std::complex<double>(I, Q);
		case 0b01: return std::complex<double>(-I, Q);
		case 0b10: return std::complex<double>(I, -Q);
		case 0b11: return std::complex<double>(-I, -Q);
		default:   return std::complex<double>(I, Q);
		}
	}
}

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(Mapper)
{
	SET_MODEL_DESCRIPTION("Complex Symbol Mapper");
	SET_MODEL_SYMBOL("SYM_Mapper");
	SET_MODEL_CATEGORY("Communications");

	ADD_MODEL_INPUT(m_input);
	ADD_MODEL_OUTPUT(m_output);

	{
		DFParam e = ADD_MODEL_ENUM_PARAM(ModType, Mapper::ModTypeEnum);
		e.SetUnit(Units::NONE);
		e.AddEnumeration("BPSK", Mapper::BPSK);
		e.AddEnumeration("QPSK", Mapper::QPSK);
		e.AddEnumeration("8-PSK", Mapper::PSK8);
		e.AddEnumeration("16-PSK", Mapper::PSK16);
		e.AddEnumeration("16-QAM", Mapper::QAM16);
		e.AddEnumeration("32-QAM", Mapper::QAM32);
		e.AddEnumeration("64-QAM", Mapper::QAM64);
		e.AddEnumeration("128-QAM", Mapper::QAM128);
		e.AddEnumeration("256-QAM", Mapper::QAM256);
		e.AddEnumeration("User Defined", Mapper::User_Defined);
		e.AddEnumeration("512-QAM", Mapper::QAM512);
		e.AddEnumeration("1024-QAM", Mapper::QAM1024);
		e.AddEnumeration("2048-QAM", Mapper::QAM2048);
		e.AddEnumeration("4096-QAM", Mapper::QAM4096);
		e.AddEnumeration("16-APSK", Mapper::APSK16);
		e.AddEnumeration("32-APSK", Mapper::APSK32);
		e.AddEnumeration("Star 16-QAM", Mapper::Star16QAM);
		e.AddEnumeration("Star 32-QAM", Mapper::Star32QAM);
		e.AddEnumeration("Custom APSK", Mapper::CustomAPSK);
		e.SetDefaultValue("QPSK");
		e.SetDescription("Modulation type");
	}

	{
		DFParam p = ADD_MODEL_PARAM(MappingTable);
		p.SetDefaultValue("[1,-1]");
		p.SetDescription("Constellation table");
		p.SetHideCondition("ModType ~= 9");
	}

	{
		DFParam p = ADD_MODEL_PARAM(Ratio_R2_R1);
		p.SetUnit(Units::NONE);
		p.SetDefaultValue("2");
		p.SetDescription("R2/R1");
		p.SetHideCondition("ModType ~= 14 && ModType ~= 15 && ModType ~= 16 && ModType ~= 17");
	}
	{
		DFParam p = ADD_MODEL_PARAM(Ratio_R3_R1);
		p.SetUnit(Units::NONE);
		p.SetDefaultValue("3");
		p.SetDescription("R3/R1");
		p.SetHideCondition("ModType ~= 15 && ModType ~= 17");
	}
	{
		DFParam p = ADD_MODEL_PARAM(Ratio_R4_R1);
		p.SetUnit(Units::NONE);
		p.SetDefaultValue("4");
		p.SetDescription("R4/R1");
		p.SetHideCondition("ModType ~= 17");
	}

	{
		DFParam p = ADD_MODEL_PARAM(RingStates);
		p.SetDefaultValue("[4;4]");
		p.SetDescription("Number of phase states on each ring");
		p.SetHideCondition("ModType ~= 18");
	}
	{
		DFParam p = ADD_MODEL_PARAM(RingMagnitudes);
		p.SetDefaultValue("[1;2]");
		p.SetDescription("Relative radius of each ring");
		p.SetHideCondition("ModType ~= 18");
	}
	{
		DFParam p = ADD_MODEL_PARAM(RinginitialPhases);
		p.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		p.SetDefaultValue("[0;0]");
		p.SetDescription("Angle of the first state on each ring relative to the x-axis");
		p.SetHideCondition("ModType ~= 18");
	}
	{
		DFParam e = ADD_MODEL_ENUM_PARAM(DefaultState, Mapper::DefaultStateEnum);
		e.SetUnit(Units::NONE);
		e.AddEnumeration("FALSE", Mapper::FALSE_);
		e.AddEnumeration("TRUE", Mapper::TRUE_);
		e.SetDefaultValue("TRUE");
		e.SetDescription("Use default states or not");
		e.SetHideCondition("ModType ~= 18");
	}
	{
		DFParam p = ADD_MODEL_PARAM(States);
		p.SetDefaultValue("[0;1;2;3;4;5;6;7]");
		p.SetDescription("States corresponding to the constellation points");
		p.SetHideCondition("ModType ~= 18 || DefaultState ~= 0");
	}

	{
		DFParam e = ADD_MODEL_ENUM_PARAM(BitOrder, Mapper::BitOrderEnum);
		e.SetUnit(Units::NONE);
		e.AddEnumeration("LSB first", Mapper::LSB_first);
		e.AddEnumeration("MSB first", Mapper::MSB_first);
		e.SetDefaultValue("LSB first");
		e.SetDescription("Bit order");
	}

	return true;
}
#endif

Mapper::Mapper()
	: ModType(QPSK)
	, BitOrder(LSB_first)
	, Ratio_R2_R1(2.0)
	, Ratio_R3_R1(3.0)
	, Ratio_R4_R1(4.0)
	, DefaultState(TRUE_)
	, m_symbolLength(2)
	, m_M(4)
{
}

bool Mapper::Initialize()
{
	return true;
}

bool Mapper::Finalize()
{
	return true;
}

unsigned int Mapper::GrayEncode(unsigned int v)
{
	return v ^ (v >> 1);
}

unsigned int Mapper::GrayDecode(unsigned int g)
{
	unsigned int b = 0u;
	while (g != 0u)
	{
		b ^= g;
		g >>= 1;
	}
	return b;
}

bool Mapper::IsPowerOfTwo(int n)
{
	return (n > 0) && ((n & (n - 1)) == 0);
}

int Mapper::ILog2Pow2(int n)
{
	int k = 0;
	while ((1 << k) < n)
		++k;
	return k;
}

double Mapper::DegToRad(double deg)
{
	return deg * SV_PI / 180.0;
}

std::complex<double> Mapper::Polar(double r, double phase)
{
	return std::complex<double>(r * std::cos(phase), r * std::sin(phase));
}

std::complex<double> Mapper::RotateByQuadrant(int q2, double I, double Q)
{
	switch (q2 & 0x3)
	{
	case 0b00: return std::complex<double>(I, Q);
	case 0b10: return std::complex<double>(-Q, I);
	case 0b11: return std::complex<double>(-I, -Q);
	case 0b01: return std::complex<double>(Q, -I);
	default:   return std::complex<double>(I, Q);
	}
}

std::complex<double> Mapper::RotateByQuadrantGray(int q2, double I, double Q)
{
	switch (q2 & 0x3)
	{
	case 0b00: return std::complex<double>(I, Q);
	case 0b01: return std::complex<double>(-Q, I);
	case 0b11: return std::complex<double>(-I, -Q);
	case 0b10: return std::complex<double>(Q, -I);
	default:   return std::complex<double>(I, Q);
	}
}

void Mapper::NormalizeTable(std::vector<std::complex<double> >& table)
{
	if (table.empty())
		return;

	double p = 0.0;
	for (size_t i = 0; i < table.size(); ++i)
		p += std::norm(table[i]);

	p /= static_cast<double>(table.size());
	if (p <= 0.0)
		return;

	const double s = 1.0 / std::sqrt(p);
	for (size_t i = 0; i < table.size(); ++i)
		table[i] *= s;
}

bool Mapper::ShouldNormalize() const
{
	return (ModType != User_Defined);
}

bool Mapper::Setup()
{
	switch (ModType)
	{
	case BPSK:      m_symbolLength = 1;  m_M = 2;    break;
	case QPSK:      m_symbolLength = 2;  m_M = 4;    break;
	case PSK8:      m_symbolLength = 3;  m_M = 8;    break;
	case PSK16:     m_symbolLength = 4;  m_M = 16;   break;
	case QAM16:     m_symbolLength = 4;  m_M = 16;   break;
	case QAM32:     m_symbolLength = 5;  m_M = 32;   break;
	case QAM64:     m_symbolLength = 6;  m_M = 64;   break;
	case QAM128:    m_symbolLength = 7;  m_M = 128;  break;
	case QAM256:    m_symbolLength = 8;  m_M = 256;  break;
	case User_Defined:
	{
		const int N = MatrixNumElements(MappingTable);
		if (!IsPowerOfTwo(N))
		{
            LOG_ERROR("User Defined: MappingTable size must be a power of 2.");
			return false;
		}
		m_M = N;
		m_symbolLength = ILog2Pow2(N);
		break;
	}
	case QAM512:    m_symbolLength = 9;  m_M = 512;  break;
	case QAM1024:   m_symbolLength = 10; m_M = 1024; break;
	case QAM2048:   m_symbolLength = 11; m_M = 2048; break;
	case QAM4096:   m_symbolLength = 12; m_M = 4096; break;
	case APSK16:    m_symbolLength = 4;  m_M = 16;   break;
	case APSK32:    m_symbolLength = 5;  m_M = 32;   break;
	case Star16QAM: m_symbolLength = 4;  m_M = 16;   break;
	case Star32QAM: m_symbolLength = 5;  m_M = 32;   break;
	case CustomAPSK:
	{
		const int nr = MatrixNumElements(RingStates);
		const int nm = MatrixNumElements(RingMagnitudes);
		const int np = MatrixNumElements(RinginitialPhases);
		if (nr <= 0 || nm != nr || np != nr)
		{
            LOG_ERROR("Custom APSK: RingStates, RingMagnitudes and RinginitialPhases must have the same non-zero length.");
			return false;
		}

		int M = 0;
		for (int i = 0; i < nr; ++i)
		{
			const int n = static_cast<int>(MatrixGetAsDouble(RingStates, i));
			if (n <= 0)
			{
                LOG_ERROR("Custom APSK: RingStates must be positive.");
				return false;
			}
			M += n;
		}

		if (!IsPowerOfTwo(M))
		{
            LOG_ERROR("Custom APSK: sum(RingStates) must be a power of 2.");
			return false;
		}

		m_M = M;
		m_symbolLength = ILog2Pow2(M);
		break;
	}
	default:
        LOG_ERROR("Unsupported ModType.");
		return false;
	}

	if ((ModType == APSK16 || ModType == APSK32 || ModType == Star16QAM || ModType == Star32QAM) && Ratio_R2_R1 <= 0.0)
	{
        LOG_ERROR("Ratio_R2_R1 must be > 0.");
		return false;
	}
	if ((ModType == APSK32 || ModType == Star32QAM) && Ratio_R3_R1 <= 0.0)
	{
        LOG_ERROR("Ratio_R3_R1 must be > 0.");
		return false;
	}
	if (ModType == Star32QAM && Ratio_R4_R1 <= 0.0)
	{
        LOG_ERROR("Ratio_R4_R1 must be > 0.");
		return false;
	}

	TrySetRate(m_input, m_symbolLength, 0);
	TrySetRate(m_output, 1, 0);

	return RebuildConstellation();
}

bool Mapper::Run()
{
	if (static_cast<int>(m_table.size()) != m_M)
		return false;

	bool bits[12] = { false };
	for (int i = 0; i < m_symbolLength; ++i)
		bits[i] = m_input[static_cast<unsigned>(i)];

	const int state = GetTableIndex(bits, m_symbolLength);
	int idx = state;

	if (ModType == CustomAPSK && DefaultState == FALSE_)
	{
		if (state < 0 || state >= m_M)
		{
            LOG_ERROR("Input state out of range.");
			return false;
		}
		idx = m_stateToIndex[static_cast<size_t>(state)];
		if (idx < 0 || idx >= m_M)
		{
            LOG_ERROR("Custom APSK state map is invalid.");
			return false;
		}
	}
	else if (state < 0 || state >= m_M)
	{
        LOG_ERROR("Input symbol value out of range.");
		return false;
	}

	m_output[0] = m_table[static_cast<size_t>(idx)];
	return true;
}

int Mapper::GetTableIndex(const bool* bits, int symbolLength) const
{
	int v = 0;
	if (BitOrder == LSB_first)
	{
		for (int i = 0; i < symbolLength; ++i)
			v |= (bits[i] ? (1 << i) : 0);
	}
	else
	{
		for (int i = 0; i < symbolLength; ++i)
			v = (v << 1) | (bits[i] ? 1 : 0);
	}
	return v;
}

bool Mapper::RebuildConstellation()
{
	std::vector<std::complex<double> > table(static_cast<size_t>(m_M), std::complex<double>(0.0, 0.0));

	switch (ModType)
	{
	case BPSK:
		BuildBPSK(table);
		break;
	case QPSK:
		BuildPSK(4, SV_PI / 4.0, table);
		break;
	case PSK8:
		BuildPSK(8, SV_PI / 8.0, table);
		break;
	case PSK16:
		BuildPSK(16, SV_PI / 16.0, table);
		break;
	case QAM16:
	case QAM32:
	case QAM64:
	case QAM128:
	case QAM256:
	case QAM512:
	case QAM1024:
	case QAM2048:
	case QAM4096:
		BuildQAM(m_M, table);
		break;
	case User_Defined:
		BuildUserDefined(table);
		break;
	case APSK16:
		BuildAPSK16(table);
		break;
	case APSK32:
		BuildAPSK32(table);
		break;
	case Star16QAM:
		BuildStar16(table);
		break;
	case Star32QAM:
		BuildStar32(table);
		break;
	case CustomAPSK:
		if (!BuildCustomAPSK(table))
			return false;
		break;
	default:
        LOG_ERROR("Unsupported ModType in RebuildConstellation.");
		return false;
	}

	if (ShouldNormalize())
		NormalizeTable(table);

	m_table.swap(table);

	if (ModType == CustomAPSK && DefaultState == FALSE_)
		return BuildStateToIndexMap(m_M);

	m_stateToIndex.clear();
	return true;
}

void Mapper::BuildBPSK(std::vector<std::complex<double> >& table) const
{
	table.assign(2, std::complex<double>(0.0, 0.0));
	table[0] = std::complex<double>(1.0, 0.0);
	table[1] = std::complex<double>(-1.0, 0.0);
}

void Mapper::BuildPSK(int M, double phase0_rad, std::vector<std::complex<double> >& table) const
{
	table.assign(static_cast<size_t>(M), std::complex<double>(0.0, 0.0));

	for (int state = 0; state < M; ++state)
	{
		const unsigned int phaseIndex = GrayDecode(static_cast<unsigned int>(state));
		const double phase = phase0_rad + 2.0 * SV_PI * static_cast<double>(phaseIndex) / static_cast<double>(M);
		table[static_cast<size_t>(state)] = Polar(1.0, phase);
	}
}

void Mapper::BuildQAM(int M, std::vector<std::complex<double> >& table) const
{
	table.assign(static_cast<size_t>(M), std::complex<double>(0.0, 0.0));

	const int k = ILog2Pow2(M);
	const int insideBits = k - 2;
	const unsigned int insideMask = (insideBits > 0) ? ((1u << insideBits) - 1u) : 0u;

	// Even k: square QAM
	if ((k & 1) == 0)
	{
		const int axisBits = (k / 2) - 1;
		const int P = 1 << axisBits;

		const bool is1024or4096 = (M == 1024 || M == 4096);
		const bool swapIQInterleave = false;
		const bool reversePositiveAxis = is1024or4096;
		const bool useGrayQuadrant = false;

		for (int state = 0; state < M; ++state)
		{
			const int q2 = (state >> insideBits) & 0x3;
			const unsigned int r = static_cast<unsigned int>(state) & insideMask;

			unsigned int iGray = 0u;
			unsigned int qGray = 0u;
			for (int b = 0; b < axisBits; ++b)
			{
				const unsigned int bitEven = (r >> (2 * b)) & 0x1u;
				const unsigned int bitOdd = (r >> (2 * b + 1)) & 0x1u;

				if (!swapIQInterleave)
				{
					iGray |= (bitEven << b);
					qGray |= (bitOdd << b);
				}
				else
				{
					qGray |= (bitEven << b);
					iGray |= (bitOdd << b);
				}
			}

			unsigned int iRank = GrayDecode(iGray);
			unsigned int qRank = GrayDecode(qGray);

			if (reversePositiveAxis)
			{
				iRank = static_cast<unsigned int>(P - 1) - iRank;
				qRank = static_cast<unsigned int>(P - 1) - qRank;
			}

			const double I = 2.0 * static_cast<double>(iRank) + 1.0;
			const double Q = 2.0 * static_cast<double>(qRank) + 1.0;

			if (is1024or4096)
			{
				table[static_cast<size_t>(state)] = ApplyQuadrantSigns1024_4096(q2, I, Q);
			}
			else
			{
				table[static_cast<size_t>(state)] =
					useGrayQuadrant ? RotateByQuadrantGray(q2, I, Q)
					: RotateByQuadrant(q2, I, Q);
			}
		}
		return;
	}

	// Odd k: cross-QAM / non-fully-square QAM (32, 128, 512, 2048)
	const int n = (k - 1) / 2;
	const int C = 1 << (n - 2);
	const int remBits = 2 * n - 4;
	const unsigned int remMask = (remBits > 0) ? ((1u << remBits) - 1u) : 0u;

	static const int BX[8] = { 0, 1, 1, 2, 0, 1, 0, 2 };
	static const int BY[8] = { 0, 0, 2, 0, 1, 1, 2, 1 };

	for (int state = 0; state < M; ++state)
	{
		const int q2 = (state >> insideBits) & 0x3;
		const unsigned int r = static_cast<unsigned int>(state) & insideMask;

		const unsigned int region = (remBits > 0) ? (r >> remBits) : r;
		const unsigned int rem = (remBits > 0) ? (r & remMask) : 0u;

		const int bx = BX[region & 0x7u];
		const int by = BY[region & 0x7u];

		const int axisLocalBits = n - 2;
		unsigned int xGray = 0u;
		unsigned int yGray = 0u;
		for (int b = 0; b < axisLocalBits; ++b)
		{
			xGray |= ((rem >> (2 * b)) & 0x1u) << b;
			yGray |= ((rem >> (2 * b + 1)) & 0x1u) << b;
		}

		unsigned int x = (axisLocalBits > 0) ? GrayDecode(xGray) : 0u;
		unsigned int y = (axisLocalBits > 0) ? GrayDecode(yGray) : 0u;

		if ((bx & 0x1) && C > 1)
			x = static_cast<unsigned int>(C - 1) - x;
		if ((by & 0x1) && C > 1)
			y = static_cast<unsigned int>(C - 1) - y;

		const int col = bx * C + static_cast<int>(x);
		const int row = by * C + static_cast<int>(y);

		const double I = 2.0 * static_cast<double>(col) + 1.0;
		const double Q = 2.0 * static_cast<double>(row) + 1.0;
		table[static_cast<size_t>(state)] = RotateByQuadrant(q2, I, Q);
	}
}

void Mapper::BuildUserDefined(std::vector<std::complex<double> >& table) const
{
	const int N = MatrixNumElements(MappingTable);
	table.assign(static_cast<size_t>(N), std::complex<double>(0.0, 0.0));
	for (int i = 0; i < N; ++i)
		table[static_cast<size_t>(i)] = MatrixGetAsComplex(MappingTable, i);
}

void Mapper::BuildAPSK16(std::vector<std::complex<double> >& table) const
{
	table.assign(16, std::complex<double>(0.0, 0.0));

	const double r1 = 1.0;
	const double r2 = Ratio_R2_R1;

	for (int state = 12; state < 16; ++state)
	{
		const unsigned int w = static_cast<unsigned int>(state - 12);
		const unsigned int k = GrayEncode(BitReverse2(w));
		table[static_cast<size_t>(state)] = Polar(r1, SV_PI / 4.0 + static_cast<double>(k) * SV_PI / 2.0);
	}

	static const int angleIndexByState[12] =
	{
		1, 10, 4, 7, 0, 11, 5, 6, 2, 9, 3, 8
	};
	for (int state = 0; state < 12; ++state)
		table[static_cast<size_t>(state)] = Polar(r2, SV_PI / 12.0 + static_cast<double>(angleIndexByState[state]) * SV_PI / 6.0);
}

void Mapper::BuildAPSK32(std::vector<std::complex<double> >& table) const
{
	table.assign(32, std::complex<double>(0.0, 0.0));

	const double r1 = 1.0;
	const double r2 = Ratio_R2_R1;
	const double r3 = Ratio_R3_R1;

	// 按 SystemVue 帮助图直接建表
	// 内环：4 点，45°,135°,225°,315°
	// 中环：12 点，15°起，每 30°
	// 外环：16 点，0°起，每 22.5°

	// ---------- inner ring ----------
	// angle order: 45, 135, 225, 315
	static const int innerStates[4] =
	{
		17, // 10001
		21, // 10101
		23, // 10111
		19  // 10011
	};

	for (int i = 0; i < 4; ++i)
	{
		const double phase = SV_PI / 4.0 + static_cast<double>(i) * SV_PI / 2.0;
		table[static_cast<size_t>(innerStates[i])] = Polar(r1, phase);
	}

	// ---------- middle ring ----------
	// angle order: 15,45,75,105,135,165,195,225,255,285,315,345 deg
	static const int middleStates[12] =
	{
		16, // 10000
		 0, // 00000
		 1, // 00001
		 5, // 00101
		 4, // 00100
		20, // 10100
		22, // 10110
		 6, // 00110
		 7, // 00111
		 3, // 00011
		 2, // 00010
		18  // 10010
	};

	for (int i = 0; i < 12; ++i)
	{
		const double phase = SV_PI / 12.0 + static_cast<double>(i) * SV_PI / 6.0;
		table[static_cast<size_t>(middleStates[i])] = Polar(r2, phase);
	}

	// ---------- outer ring ----------
	// angle order: 0,22.5,45,67.5,...,337.5 deg
	static const int outerStates[16] =
	{
		24, // 11000
		 8, // 01000
		25, // 11001
		 9, // 01001
		13, // 01101
		29, // 11101
		12, // 01100
		28, // 11100
		30, // 11110
		14, // 01110
		31, // 11111
		15, // 01111
		11, // 01011
		27, // 11011
		10, // 01010
		26  // 11010
	};

	for (int i = 0; i < 16; ++i)
	{
		const double phase = static_cast<double>(i) * SV_PI / 8.0;
		table[static_cast<size_t>(outerStates[i])] = Polar(r3, phase);
	}
}

void Mapper::BuildStar16(std::vector<std::complex<double> >& table) const
{
	table.assign(16, std::complex<double>(0.0, 0.0));

	const double r1 = 1.0;
	const double r2 = Ratio_R2_R1;

	for (int state = 0; state < 16; ++state)
	{
		const int b0 = (state >> 0) & 1;
		const int b1 = (state >> 1) & 1;
		const int b2 = (state >> 2) & 1;
		const int b3 = (state >> 3) & 1;

		const int g2 = b3;
		const int g1 = b1 ^ b3;
		const int g0 = b1 ^ b2 ^ b3;
		const int phaseIndex = (g0) | (g1 << 1) | (g2 << 2);

		const double radius = (b0 == 0) ? r1 : r2;
		table[static_cast<size_t>(state)] = Polar(radius, static_cast<double>(phaseIndex) * SV_PI / 4.0);
	}
}

void Mapper::BuildStar32(std::vector<std::complex<double> >& table) const
{
	table.assign(32, std::complex<double>(0.0, 0.0));

	const double r1 = 1.0;
	const double r2 = Ratio_R2_R1;
	const double r3 = Ratio_R3_R1;
	const double r4 = Ratio_R4_R1;

	static const double ringByLow2[4] =
	{
		r1,
		r2,
		r4,
		r3
	};

	static const int phaseIndexByGroup[8] =
	{
		0,
		3,
		1,
		2,
		7,
		4,
		6,
		5
	};

	for (int state = 0; state < 32; ++state)
	{
		const int ringSel = state & 0x3;
		const int group = (state >> 2) & 0x7;

		const double radius = ringByLow2[ringSel];
		const int phaseIndex = phaseIndexByGroup[group];

		table[static_cast<size_t>(state)] =
			Polar(radius, static_cast<double>(phaseIndex) * SV_PI / 4.0);
	}
}

bool Mapper::BuildCustomAPSK(std::vector<std::complex<double> >& table)
{
	const int nr = MatrixNumElements(RingStates);
	const int nm = MatrixNumElements(RingMagnitudes);
	const int np = MatrixNumElements(RinginitialPhases);

	if (nr <= 0 || nm != nr || np != nr)
	{
        LOG_ERROR("Custom APSK: RingStates, RingMagnitudes and RinginitialPhases must have the same length.");
		return false;
	}

	table.clear();
	table.reserve(static_cast<size_t>(m_M));

	for (int r = 0; r < nr; ++r)
	{
		const int nStates = static_cast<int>(MatrixGetAsDouble(RingStates, r));
		const double mag = MatrixGetAsDouble(RingMagnitudes, r);
		const double phase0 = DegToRad(MatrixGetAsDouble(RinginitialPhases, r));

		if (nStates <= 0)
		{
            LOG_ERROR("Custom APSK: RingStates must be positive.");
			return false;
		}
		if (mag < 0.0)
		{
            LOG_ERROR("Custom APSK: RingMagnitudes must be non-negative.");
			return false;
		}

		for (int k = 0; k < nStates; ++k)
		{
			const double phase = phase0 + 2.0 * SV_PI * static_cast<double>(k) / static_cast<double>(nStates);
			table.push_back(Polar(mag, phase));
		}
	}

	if (static_cast<int>(table.size()) != m_M)
	{
        LOG_ERROR("Custom APSK: total generated points does not equal constellation size.");
		return false;
	}

	return true;
}

bool Mapper::BuildStateToIndexMap(int M)
{
	const int n = MatrixNumElements(States);
	if (n != M)
	{
        LOG_ERROR("States length must equal the constellation size.");
		return false;
	}

	m_stateToIndex.assign(static_cast<size_t>(M), -1);
	std::set<int> used;

	for (int i = 0; i < M; ++i)
	{
		const int s = static_cast<int>(MatrixGetAsDouble(States, i));
		if (s < 0 || s >= M)
		{
            LOG_ERROR("States contains an out-of-range value.");
			return false;
		}
		if (!used.insert(s).second)
		{
            LOG_ERROR("States must be a permutation of 0..M-1.");
			return false;
		}
		m_stateToIndex[static_cast<size_t>(s)] = i;
	}

	return true;
}
