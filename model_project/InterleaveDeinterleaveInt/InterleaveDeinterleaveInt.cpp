#include "InterleaveDeinterleaveInt.h"

#ifndef SV_CODE_GEN
namespace SystemVueModelBuilder
{
	DEFINE_MODEL_INTERFACE(InterleaveDeinterleaveInt)
	{
		SET_MODEL_DESCRIPTION("Interleaver /Deinterleaver");
		SET_MODEL_SYMBOL("SYM_InterleaveDeinterleave");
		SET_MODEL_CATEGORY("Communications");

		// 输入端口
		{
			DFPort p = ADD_MODEL_INPUT(input);
			p.SetName("input");
			p.SetDescription("input Signal");
		}

		// 输出端口
		{
			DFPort p = ADD_MODEL_OUTPUT(output);
			p.SetName("output");
			p.SetDescription("output Signal");
		}

		// Rows
		{
			DFParam param = ADD_MODEL_PARAM(Rows);
			param.SetName("Rows");
			param.SetUnit(Units::NONE);
			param.SetDefaultValue("8");
			param.SetDescription("Number of rows of the interleave/deinterleave matrix");
		}

		// Columns
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
	InterleaveDeinterleaveInt::InterleaveDeinterleaveInt()
		: Rows(8), Columns(8)
	{
	}

	bool InterleaveDeinterleaveInt::Setup()
	{
		if (Columns > 0 && Rows > 0)
		{
			const size_t N = static_cast<size_t>(Rows) * static_cast<size_t>(Columns);
			input.SetRate(static_cast<unsigned>(N));
			output.SetRate(static_cast<unsigned>(N));
		}
		return true;
	}

	bool InterleaveDeinterleaveInt::Initialize()
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
		return bStatus;
	}

	bool InterleaveDeinterleaveInt::Run()
	{
		for (size_t iIndex = static_cast<size_t>(Columns); iIndex > 0; --iIndex)
		{
			const size_t i = iIndex - 1;

			for (int jIndex = Rows; jIndex > 0; --jIndex)
			{
				const size_t j = static_cast<size_t>(jIndex - 1);

				input.Copy(i + j * static_cast<size_t>(Columns),
					&output,
					j + i * static_cast<size_t>(Rows),
					1);
			}
		}
		return true;
	}
}
