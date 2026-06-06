#include "RADAR_MTI_M.h"

// 以下代码仅在非代码生成环境中编译
#ifndef SV_CODE_GEN

// 定义SystemVue模型接口
DEFINE_MODEL_INTERFACE(RADAR_MTI_M)
{
	SET_MODEL_DESCRIPTION("Moving Target Indication for Matrix signals");
	SET_MODEL_SYMBOL("SYM_RADAR_MTI_M@RADAR Symbols");
	SET_MODEL_CATEGORY("Signal Processing");

	// 添加输入端口：complex matrix
	{
		SystemVueModelBuilder::DFPort p = ADD_MODEL_INPUT(input);
		p.SetDescription("The input signal");
	}

	// 添加输出端口：complex matrix
	{
		SystemVueModelBuilder::DFPort p = ADD_MODEL_OUTPUT(output);
		p.SetDescription("The output signal after MTI processing");
	}

	// 添加MTI滤波器类型枚举参数
	{
		SystemVueModelBuilder::DFParam P1 = ADD_MODEL_ENUM_PARAMETER(MTI_Type, SelectedMTI_Type);

		P1.AddEnumeration("Two Pulse Canceller", TwoPulseCanceller);
		P1.AddEnumeration("Three Pulse Canceller", ThreePulseCanceller);
		P1.SetDefaultValue("0");
		P1.SetDescription("The Type of Moving Target Indicator: Two Pulse Canceller, Three Pulse Canceller");
	}

	return true;
}

#endif

// 构造函数
RADAR_MTI_M::RADAR_MTI_M()
	: MTI_Type(TwoPulseCanceller)
{
}

// 初始化函数
bool RADAR_MTI_M::Setup()
{
	// 矩阵版一次输入/输出都是1个Matrix
	input.SetRate(1);
	output.SetRate(1);

	return true;
}

//-----------------------------------------------------------------------------------
//	Run - 主运行函数
//		矩阵版MTI处理
//
//		默认矩阵含义：
//		行方向：距离门 / 一个脉冲内的采样点
//		列方向：脉冲序号
//
//		Two Pulse Canceller:
//			y[p] = x[p] - x[p-1]
//
//		Three Pulse Canceller:
//			y[p] = x[p] - 2*x[p-1] + x[p-2]
//-----------------------------------------------------------------------------------
bool RADAR_MTI_M::Run()
{
	// 读取输入矩阵
	SystemVueModelBuilder::Matrix< std::complex<double> >& inMat = input[0];

	// 获取矩阵维度
	const int nRows = static_cast<int>(inMat.NumRows());
	const int nCols = static_cast<int>(inMat.NumColumns());

	if (nRows <= 0 || nCols <= 0)
	{
		return false;
	}

	if (MTI_Type == TwoPulseCanceller && nCols < 2)
	{
		return false;
	}

	if (MTI_Type == ThreePulseCanceller && nCols < 3)
	{
		return false;
	}

	SystemVueModelBuilder::Matrix< std::complex<double> > outMat;

	switch (MTI_Type)
	{
	case TwoPulseCanceller:
	{
		const int outCols = nCols - 1;
		outMat.Resize(nRows, outCols);

		for (int row = 0; row < nRows; ++row)
		{
			for (int pulse = 1; pulse < nCols; ++pulse)
			{
				const int outCol = pulse - 1;

				// 当前脉冲减去前一个脉冲
				outMat(row, outCol) = inMat(row, pulse) - inMat(row, pulse - 1);
			}
		}
		break;
	}

	case ThreePulseCanceller:
	{
		const int outCols = nCols - 2;
		outMat.Resize(nRows, outCols);

		for (int row = 0; row < nRows; ++row)
		{
			for (int pulse = 2; pulse < nCols; ++pulse)
			{
				const int outCol = pulse - 2;

				// 当前脉冲 - 2×前一个脉冲 + 前两个脉冲
				outMat(row, outCol) =
					inMat(row, pulse)
					- inMat(row, pulse - 1) * 2.0
					+ inMat(row, pulse - 2);
			}
		}
		break;
	}

	default:
		return false;
	}

	output[0] = outMat;

	return true;
}