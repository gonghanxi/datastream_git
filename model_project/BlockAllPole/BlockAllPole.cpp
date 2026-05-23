#include "BlockAllPole.h"

#ifndef SV_CODE_GEN
namespace SystemVueModelBuilder {

	DEFINE_MODEL_INTERFACE(BlockAllPole)
	{
		SET_MODEL_DESCRIPTION("AllPole Filter for Data Blocks");
		SET_MODEL_SYMBOL("SYM_BlockAllPole");
		SET_MODEL_CATEGORY("Filters");

		{
			DFPort p = ADD_MODEL_INPUT(m_signalIn);
			p.SetName("signalIn");
			p.SetDescription("Input signal (real). Block input.");
		}

		{
			DFPort p = ADD_MODEL_INPUT(m_coefs);
			p.SetName("coefs");
			p.SetDescription("Denominator coefficients d1..dM (real). Read Order new coefficients each firing.");
		}

		{
			DFPort p = ADD_MODEL_OUTPUT(m_signalOut);
			p.SetName("signalOut");
			p.SetDescription("Output signal (real). Block output.");
		}

		{
			DFParam param = ADD_MODEL_PARAM(m_blockSize);
			param.SetName("BlockSize");
			param.SetDescription("Number of inputs that use each coefficient set");
			param.SetDefaultValue("128");
		}

		{
			DFParam param = ADD_MODEL_PARAM(m_order);
			param.SetName("Order");
			param.SetDescription("Number of new coefficients to read each time");
			param.SetDefaultValue("16");
		}

		return true;
	}

} // 
#endif // SV_CODE_GEN

namespace SystemVueModelBuilder {

	BlockAllPole::BlockAllPole()
		: m_blockSize(128),
		m_order(16),
		m_delay(0)
	{
	}

	bool BlockAllPole::ValidateParameters()
	{
		if (m_blockSize < 1)
		{
			POST_ERROR("BlockSize must be > 0.");
			return false;
		}
		if (m_order < 1)
		{
			POST_ERROR("Order must be > 0.");
			return false;
		}

		if (m_taps.size() != static_cast<std::size_t>(m_order))
			m_taps.assign(static_cast<std::size_t>(m_order), 0.0);

		if (m_delayLine.size() != static_cast<std::size_t>(m_order))
			m_delayLine.assign(static_cast<std::size_t>(m_order), 0.0); // 初始历史输出为 0

		return true;
	}

	bool BlockAllPole::Setup()
	{
		if (!ValidateParameters())
			return false;

		// 每次 firing：输入读 BlockSize，系数读 Order，输出写 BlockSize
		m_signalIn.SetRate(static_cast<unsigned>(m_blockSize));
		m_coefs.SetRate(static_cast<unsigned>(m_order));
		m_signalOut.SetRate(static_cast<unsigned>(m_blockSize));

		return true;
	}

	bool BlockAllPole::Run()
	{
		if (!ValidateParameters())
			return false;

		// 1) 读取本块新系数：d1..dM
		for (int k = 0; k < m_order; ++k)
		{
			m_taps[static_cast<std::size_t>(k)] = m_coefs[k];
		}

		// 2) 用本组系数处理一整块输入
		// 差分方程：y[n] = x[n] + sum_{k=1..M} d_k * y[n-k]
		for (int n = 0; n < m_blockSize; ++n)
		{
			const double x = m_signalIn[n];

			double y = x;
			for (int k = 0; k < m_order; ++k)
			{
				// k=0 -> d1 * y[n-1]
				y += m_taps[static_cast<std::size_t>(k)] * m_delayLine[static_cast<std::size_t>(k)];
			}

			m_signalOut[n] = y;

			// 更新延迟线：y[n] 进入 y[n-1] 位置，其余后移
			for (int k = m_order - 1; k >= 1; --k)
			{
				m_delayLine[static_cast<std::size_t>(k)] = m_delayLine[static_cast<std::size_t>(k - 1)];
			}
			m_delayLine[0] = y;
		}

		return true;
	}

} 
