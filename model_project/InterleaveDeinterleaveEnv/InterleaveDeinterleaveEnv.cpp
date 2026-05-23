#include "InterleaveDeinterleaveEnv.h"

#ifndef SV_CODE_GEN
namespace SystemVueModelBuilder
{
	DEFINE_MODEL_INTERFACE(InterleaveDeinterleaveEnv)
	{
		SET_MODEL_DESCRIPTION("Interleaver /Deinterleaver (Envelope)");
		SET_MODEL_SYMBOL("SYM_InterleaveDeinterleave");
		SET_MODEL_CATEGORY("Communications");

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

		{
			DFParam param = ADD_MODEL_PARAM(Rows);
			param.SetName("Rows");
			param.SetUnit(Units::NONE);
			param.SetDefaultValue("8");
			param.SetDescription("Number of rows of the interleave/deinterleave matrix");
		}

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
#endif


namespace SystemVueModelBuilder
{
	InterleaveDeinterleaveEnv::InterleaveDeinterleaveEnv()
		: Rows(8),
		Columns(8),
		m_blockSize(64u)
	{
	}

	bool InterleaveDeinterleaveEnv::Setup()
	{
		if (Rows > 0 && Columns > 0)
		{
			const size_t N = static_cast<size_t>(Rows) * static_cast<size_t>(Columns);

			m_blockSize = static_cast<unsigned>(N);

			input.SetRate(m_blockSize);
			output.SetRate(m_blockSize);
		}
		return true;
	}

	bool InterleaveDeinterleaveEnv::Initialize()
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

		if (bStatus)
		{
			const size_t N = static_cast<size_t>(Rows) * static_cast<size_t>(Columns);
			m_blockSize = static_cast<unsigned>(N);
		}

		return bStatus;
	}

	bool InterleaveDeinterleaveEnv::Run()
	{
		for (size_t iIndex = static_cast<size_t>(Columns); iIndex > 0; --iIndex)
		{
			const size_t i = iIndex - 1;

			for (int jIndex = Rows; jIndex > 0; --jIndex)
			{
				const size_t j = static_cast<size_t>(jIndex - 1);

				// 只改为 Envelope 端口类型，其余逻辑完全一致
				input.Copy(i + j * static_cast<size_t>(Columns), &output,
					j + i * static_cast<size_t>(Rows), 1);
			}
		}

		return true;
	}
}
