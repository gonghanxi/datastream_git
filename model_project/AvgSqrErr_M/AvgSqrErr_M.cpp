#include "AvgSqrErr_M.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(AvgSqrErr_M)
{
	SET_MODEL_DESCRIPTION("Mean Squared Error Matrix Averager");
	SET_MODEL_SYMBOL("SYM_AvgSqrErr_M");
	SET_MODEL_CATEGORY("Math Matrix");

	{
		SystemVueModelBuilder::DFPort p = ADD_MODEL_INPUT(input1);
		p.SetName("input1");
		p.SetDescription("input matrix 1");
	}

	{
		SystemVueModelBuilder::DFPort p = ADD_MODEL_INPUT(input2);
		p.SetName("input2");
		p.SetDescription("input matrix 2");
	}

	{
		SystemVueModelBuilder::DFPort p = ADD_MODEL_OUTPUT(output);
		p.SetName("output");
		p.SetDescription("output averaged squared error");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(NumInputsToAverage);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("8");
		p.SetDescription("Number of input matrices to average");
	}

	return true;
}
#endif

AvgSqrErr_M::AvgSqrErr_M()
	: input1()
	, input2()
	, output()
	, NumInputsToAverage(8)
{
}

bool AvgSqrErr_M::Setup()
{
	if (NumInputsToAverage < 1)
	{
		POST_ERROR("AvgSqrErr_M: NumInputsToAverage must be >= 1.");
		return false;
	}

	// 内置帮助文档说明：
	// For every NumInputs matrices from each input port,
	// one non-negative float value is output.
	//
	// 因此不是滑动平均，而是：
	// 每次从 input1 读取 NumInputsToAverage 个矩阵，
	// 每次从 input2 读取 NumInputsToAverage 个矩阵，
	// 输出 1 个 real 标量。
	input1.SetRate(static_cast<unsigned>(NumInputsToAverage));
	input2.SetRate(static_cast<unsigned>(NumInputsToAverage));
	output.SetRate(1U);

	return true;
}

bool AvgSqrErr_M::Run()
{
	double totalSSE = 0.0;

	for (int n = 0; n < NumInputsToAverage; ++n)
	{
		const SystemVueModelBuilder::Matrix<double>& A =
			input1[static_cast<unsigned>(n)];

		const SystemVueModelBuilder::Matrix<double>& B =
			input2[static_cast<unsigned>(n)];

		if (A.NumRows() != B.NumRows() ||
			A.NumColumns() != B.NumColumns())
		{
			POST_ERROR("AvgSqrErr_M: input1 and input2 matrices must have identical sizes.");
			output[0U] = 0.0;
			return false;
		}

		const std::size_t numElements = A.NumElements();

		double pairSSE = 0.0;

		for (std::size_t i = 0; i < numElements; ++i)
		{
			const double d = A(i) - B(i);
			pairSSE += d * d;
		}

		// 每一对矩阵先把所有对应元素的平方误差求和。
		totalSSE += pairSSE;
	}

	// 然后对 NumInputsToAverage 个矩阵对的平方误差和取平均。
	// 注意：根据帮助文档，不再除以矩阵元素个数。
	output[0U] = totalSSE / static_cast<double>(NumInputsToAverage);

	return true;
}