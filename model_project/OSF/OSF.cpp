#include "OSF.h"

#include <algorithm>
#include <cmath>
#include <numeric>

#ifndef SV_CODE_GEN

namespace SystemVueModelBuilder {

	namespace {
		template <typename P>
		static auto SetRuntimeTunableIfSupported(P& p, bool v) -> decltype(p.SetRuntimeTunable(v), void())
		{
			p.SetRuntimeTunable(v);
		}

		static void SetRuntimeTunableIfSupported(...)
		{
		}
	}

	DEFINE_MODEL_INTERFACE(OSF)
	{
		SET_MODEL_DESCRIPTION("Order Statstic Filter"); 
		SET_MODEL_SYMBOL("SYM_OSF");
		SET_MODEL_CATEGORY("Filters");

		// 端口
		{
			DFPort inPort = ADD_MODEL_INPUT(m_input);
			inPort.SetName("input");
			inPort.SetDescription("input");
		}
		{
			DFPort outPort = ADD_MODEL_OUTPUT(m_output);
			outPort.SetName("output");
			outPort.SetDescription("output");
		}

		// 参数 N
		{
			DFParam p = ADD_MODEL_PARAM(m_n);
			p.SetName("N");
			p.SetUnit(Units::NONE);
			p.SetDefaultValue("3");
			p.SetDescription("Size of sliding window");

			SetRuntimeTunableIfSupported(p, false);
		}

		// 参数 Percentile
		{
			DFParam p = ADD_MODEL_PARAM(m_percentile);
			p.SetName("Percentile");
			p.SetUnit(Units::NONE);
			p.SetDefaultValue("50");
			p.SetDescription("Ranking percentile (0 percent is the minimum)");

			// 内置：Percentile 运行时可调（如果 SDK 支持该接口则设置）
			SetRuntimeTunableIfSupported(p, true);
		}

		return true;
	}

} 
#endif 


namespace SystemVueModelBuilder {

	OSF::OSF()
		: m_input(0.0),
		m_output(0.0),
		m_n(3),
		m_percentile(50),
		m_current(0),
		m_index(0)
	{
	}

	bool OSF::ValidateParameters()
	{
		if (m_n < 1)
		{
			POST_ERROR("N must be > 0.");
			return false;
		}

		// Percentile 合法区间：0..100
		if (m_percentile < 0)
		{
			POST_WARNING("Percentile < 0, clamped to 0.");
			m_percentile = 0;
		}
		else if (m_percentile > 100)
		{
			POST_WARNING("Percentile > 100, clamped to 100.");
			m_percentile = 100;
		}

		// i = round(p*(N-1)/100)
		const double x = (static_cast<double>(m_percentile) * static_cast<double>(m_n - 1)) / 100.0;
		int idx = static_cast<int>(std::floor(x + 0.5));

		if (idx < 0) idx = 0;
		if (idx > (m_n - 1)) idx = (m_n - 1);

		m_index = idx;
		return true;
	}

	bool OSF::Setup()
	{
		if (!ValidateParameters())
			return false;

		m_window.assign(static_cast<std::size_t>(m_n), 0.0);

		m_ranks.resize(static_cast<std::size_t>(m_n));
		for (int i = 0; i < m_n; ++i)
			m_ranks[static_cast<std::size_t>(i)] = i;

		// 初始窗口全 0，输出自然为 0
		m_current = 0;
		m_output = 0.0;

		return true;
	}

	bool OSF::UpdateDynamicParameters()
	{
		// 预期只有 Percentile 会在运行时调参触发
		const int oldN = static_cast<int>(m_window.size());

		if (!ValidateParameters())
			return false;

		if (oldN != m_n)
		{
			m_window.assign(static_cast<std::size_t>(m_n), 0.0);
			m_ranks.resize(static_cast<std::size_t>(m_n));
			for (int i = 0; i < m_n; ++i)
				m_ranks[static_cast<std::size_t>(i)] = i;

			m_current = 0;
			m_output = 0.0;
		}

		return true;
	}

	void OSF::OrderedInsert()
	{
		const int idx = m_current;

		{
			auto it = std::find(m_ranks.begin(), m_ranks.end(), idx);
			if (it != m_ranks.end())
				m_ranks.erase(it);
		}

		// 2) 按 (value, index) 重新插入，保证确定性（值相等按下标排）
		const double v = m_window[static_cast<std::size_t>(idx)];

		auto lessByValueThenIndex = [&](int a, int b) -> bool
		{
			const double va = m_window[static_cast<std::size_t>(a)];
			const double vb = m_window[static_cast<std::size_t>(b)];
			if (va < vb) return true;
			if (va > vb) return false;
			return a < b;
		};

		// lower_bound：找到第一个 >= idx(按自定义顺序) 的位置
		auto pos = std::lower_bound(
			m_ranks.begin(),
			m_ranks.end(),
			idx,
			[&](int a, int b) -> bool { return lessByValueThenIndex(a, b); }
		);

		m_ranks.insert(pos, idx);
	}

	bool OSF::Run()
    {
		// 写入当前样本到窗口（环形覆盖最旧样本）
		m_window[static_cast<std::size_t>(m_current)] = m_input;

		// 更新排序索引
		OrderedInsert();

		// 输出第 m_index 个顺序统计量（0..N-1）
		const int outWinIdx = m_ranks[static_cast<std::size_t>(m_index)];
		m_output = m_window[static_cast<std::size_t>(outWinIdx)];


		// 前进写指针
		++m_current;
		if (m_current >= m_n)
			m_current = 0;

		return true;
	}

} 
