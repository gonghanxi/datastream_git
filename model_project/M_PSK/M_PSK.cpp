#include "M_PSK.h"

#include <cmath>
#include <algorithm>

#ifndef SV_CODE_GEN

namespace SystemVueModelBuilder {

	DEFINE_MODEL_INTERFACE(M_PSK)
	{
		SET_MODEL_DESCRIPTION("Complex PSK symbol mapper");
		SET_MODEL_SYMBOL("SYM_M_PSK");
		SET_MODEL_CATEGORY("Communications");

		{
			DFPort inPort = ADD_MODEL_INPUT(m_in);
			inPort.SetName("In");
			inPort.SetDescription("Input bit sequence (int). Even->0, Odd->1.");
		}

		{
			DFPort outPort = ADD_MODEL_OUTPUT(m_out);
			outPort.SetName("Out");
			outPort.SetDescription("Output complex symbol.");
		}

		{
			DFParam p = ADD_MODEL_ENUM_PARAM(m_modType, ModType);
			p.SetName("ModType");
			p.SetUnit(Units::NONE);
			p.SetDescription("Modulation type");

			p.AddEnumeration("BPSK", BPSK);
			p.AddEnumeration("QPSK", QPSK);
			p.AddEnumeration("PSK8", PSK8);
			p.AddEnumeration("PSK16", PSK16);
			p.AddEnumeration("PSK32", PSK32);
			p.AddEnumeration("PSK64", PSK64);
			p.AddEnumeration("PSK128", PSK128);
			p.AddEnumeration("PSK256", PSK256);
			p.AddEnumeration("PSK512", PSK512);

			p.SetDefaultValue("QPSK");
		}

		{
			DFParam p = ADD_MODEL_ENUM_PARAM(m_bitOrder, BitOrder);
			p.SetName("BitOrder");
			p.SetUnit(Units::NONE);
			p.SetDescription("Bit order");

			p.AddEnumeration("LSB first", LSB_first);
			p.AddEnumeration("MSB first", MSB_first);

			p.SetDefaultValue("MSB first");
		}

		return true;
	}

} 
#endif 


namespace SystemVueModelBuilder {

	M_PSK::M_PSK()
		: m_out(0.0, 0.0),
		m_modType(QPSK),
		m_bitOrder(MSB_first),
		m_modOrder(4),
		m_modBits(2),
		m_angleStep(0.0f),
		m_setupModType(QPSK)
	{
	}

	bool M_PSK::ValidateParameters()
	{
		if (m_modType < BPSK || m_modType >= MOD_COUNT)
		{
			POST_ERROR("ModType is out of range.");
			return false;
		}
		if (m_bitOrder != LSB_first && m_bitOrder != MSB_first)
		{
			POST_ERROR("BitOrder is out of range.");
			return false;
		}

		m_modBits = static_cast<std::size_t>(m_modType) + 1;
		m_modOrder = static_cast<std::size_t>(1u) << static_cast<unsigned>(m_modBits);

		if (m_modBits < 1 || m_modBits > 9 || m_modOrder > 512)
		{
			POST_ERROR("Derived modulation order is invalid (supported: 2..512).");
			return false;
		}

		return true;
	}

	void M_PSK::GenerateGrayCoding()
	{
		m_grayTable.assign(m_modOrder, 0);

		for (std::size_t k = 0; k < m_modOrder; ++k)
		{
			std::size_t g = k ^ (k >> 1);
			m_grayTable[g] = static_cast<int>(k);
		}
	}

	int M_PSK::ConvertBitsToInt(const std::vector<int>& bits) const
	{
		int value = 0;

		if (m_bitOrder == MSB_first)
		{
			for (std::size_t i = 0; i < bits.size(); ++i)
				value = (value << 1) | (bits[i] & 1);
		}
		else
		{
			for (std::size_t i = 0; i < bits.size(); ++i)
				value |= ((bits[i] & 1) << static_cast<int>(i));
		}

		return value;
	}

	void M_PSK::BuildConstellationTable()
	{
		// 用 float 域生成星座表（贴近内置：float pi + float step + float angle=step*k + cosf/sinf）
		const float pi_f = acosf(-1.0f);
		const float twoPi_f = 2.0f * pi_f;

		// 先算 step，再 angle=step*k
		m_angleStep = twoPi_f / static_cast<float>(m_modOrder);

		m_constTable.resize(m_modOrder);
		for (std::size_t k = 0; k < m_modOrder; ++k)
		{
			const float angle = m_angleStep * static_cast<float>(k);
			m_constTable[k] = std::complex<float>(cosf(angle), sinf(angle));
		}
	}

	bool M_PSK::Setup()
	{
		if (!ValidateParameters())
			return false;

		m_in.SetRate(static_cast<unsigned>(m_modBits));

		GenerateGrayCoding();
		BuildConstellationTable();

		m_setupModType = m_modType;

		return true;
	}

	bool M_PSK::Initialize()
	{
		if (!ValidateParameters())
			return false;

		if (m_modType != m_setupModType)
		{
			POST_ERROR("ModType changed after Setup. This is not supported (would require rate/table rebuild).");
			return false;
		}

		return true;
	}

	bool M_PSK::UpdateDynamicParameters()
	{
		if (!ValidateParameters())
			return false;

		if (m_modType != m_setupModType)
		{
			POST_ERROR("Runtime ModType change is not supported (rate/table would need to change).");
			return false;
		}

		return true;
	}

	bool M_PSK::Run()
	{
		// 读取 n 个输入 token，并按奇偶解释为 bit：偶->0，奇->1
		std::vector<int> bits(m_modBits, 0);
		for (std::size_t i = 0; i < m_modBits; ++i)
		{
			const int x = m_in[static_cast<int>(i)];
			bits[i] = (x & 1) ? 1 : 0;
		}

		const int g = ConvertBitsToInt(bits);

		const int k = m_grayTable[static_cast<std::size_t>(g)];

		if (k < 0 || static_cast<std::size_t>(k) >= m_constTable.size())
		{
			POST_ERROR("Internal constellation table index out of range.");
			return false;
		}

		const std::complex<float>& c = m_constTable[static_cast<std::size_t>(k)];
		m_out = std::complex<double>((double)c.real(), (double)c.imag());

		return true;
	}

} 
