#include "InterleaveDeinterleave.h"

#ifndef SV_CODE_GEN
namespace SystemVueModelBuilder
{
	DEFINE_MODEL_INTERFACE(InterleaveDeinterleave)
	{
		//-------------------- 预设（按你要求） --------------------
		SET_MODEL_DESCRIPTION("Interleaver /Deinterleaver");
		SET_MODEL_SYMBOL("SYM_InterleaveDeinterleave");
		SET_MODEL_CATEGORY("Communications");

		//-------------------- 端口 --------------------
		{
			DFPort p = ADD_MODEL_INPUT(input);
			p.SetName("input");
			p.SetDescription("input Signal");
		}

		{
			DFPort p = ADD_MODEL_OUTPUT(output);
			p.SetName("output");
			p.SetDescription("output Signal");
		}

		//-------------------- 参数：Rows --------------------
		{
			DFParam param = ADD_MODEL_PARAM(Rows);
			param.SetName("Rows");
			param.SetUnit(Units::NONE);
			param.SetDefaultValue("8");
			param.SetDescription("Number of rows of the interleave/deinterleave matrix");
		}

		//-------------------- 参数：Columns --------------------
		{
			DFParam param = ADD_MODEL_PARAM(Columns);
			param.SetName("Columns");
			param.SetUnit(Units::NONE);
			param.SetDefaultValue("8");
			param.SetDescription("Number of columns of the interleave/deinterleave matrix");
		}

		return true;
	}
}
#endif // SV_CODE_GEN


namespace SystemVueModelBuilder
{
	InterleaveDeinterleave::InterleaveDeinterleave()
		: Rows(8),
		Columns(8),
		m_blockSize(64u)
	{
	}

	bool InterleaveDeinterleave::Setup()
	{
		// 与内置模板一致：Rows/Columns>0 时设置块速率 N=Rows*Columns
		if (Rows > 0 && Columns > 0)
		{
			// 注意：与内置一致，不在这里做过多“修正”，只做正数判断
			const size_t N = static_cast<size_t>(Rows) * static_cast<size_t>(Columns);

			// SystemVue SetRate 通常是 unsigned
			m_blockSize = static_cast<unsigned>(N);

			input.SetRate(m_blockSize);
			output.SetRate(m_blockSize);
		}
		return true;
	}

	bool InterleaveDeinterleave::Initialize()
	{
		bool bStatus = true;

		if (Rows < 1)
		{
			POST_ERROR("Number of rows must be > 0 ");
			bStatus = false;
		}
		if (Columns < 1)
		{
			POST_ERROR("Number of columns must be > 0 ");
			bStatus = false;
		}

		// 计算块长度（便于 Run 使用）
		if (bStatus)
		{
			const size_t N = static_cast<size_t>(Rows) * static_cast<size_t>(Columns);
			m_blockSize = static_cast<unsigned>(N);
		}

		return bStatus;
	}

	bool InterleaveDeinterleave::Run()
	{
		// 完全对齐你给的内置模板 Run()：
		// for i in [0..Columns-1], for j in [0..Rows-1]:
		//    output[j + i*Rows] = input[i + j*Columns]
		//
		// 内置写法用 Copy(i+j*Columns, &output, j+i*Rows, 1)

		for (size_t iIndex = static_cast<size_t>(Columns); iIndex > 0; --iIndex)
		{
			const size_t i = iIndex - 1;

			for (int jIndex = Rows; jIndex > 0; --jIndex)
			{
				const size_t j = static_cast<size_t>(jIndex - 1);

				// 使用 Copy：最大化与内置一致
				input.Copy(i + j * static_cast<size_t>(Columns), &output,
					j + i * static_cast<size_t>(Rows), 1);
			}
		}

		return true;
	}
}
