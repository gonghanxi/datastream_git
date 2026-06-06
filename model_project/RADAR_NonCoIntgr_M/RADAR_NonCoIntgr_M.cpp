#include "RADAR_NonCoIntgr_M.h"

#ifndef SV_CODE_GEN

DEFINE_MODEL_INTERFACE(RADAR_NonCoIntgr_M)
{
	// ===== 模型基本信息 =====
	SET_MODEL_DESCRIPTION("Signal non-coherent Integration for Matrix signals");
	SET_MODEL_SYMBOL("SYM_RADAR_NonCoIntgr_M@RADAR Symbols");
	SET_MODEL_CATEGORY("Signal Processing");

	// ===== 输入端口：complex matrix =====
	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(input);
		port.SetDescription("The input signal");
	}

	// ===== 输出端口：real matrix =====
	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
		port.SetDescription("The output signal after non-coherent integration");
	}

	// ===== 参数：Number =====
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(Number);
		param.SetDescription("Number of Pulses for non-coherent integration");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("5");
	}

	return true;
}

#endif


//------------------------------------------------------------------------------
// 构造函数
//------------------------------------------------------------------------------
RADAR_NonCoIntgr_M::RADAR_NonCoIntgr_M()
	: Number(5)
{
}


//------------------------------------------------------------------------------
// Setup
// 由于一次非相干积分的数据已经封装在一个 Matrix 中，因此不需要 SampleRate 参数。
//------------------------------------------------------------------------------
bool RADAR_NonCoIntgr_M::Setup()
{
	if (Number <= 0)
	{
		return false;
	}

	input.SetRate(1);
	output.SetRate(1);

	return true;
}


//------------------------------------------------------------------------------
// Run
//
// 1. 如果矩阵行数 == Number：
//      认为每一行是一帧 / 一个脉冲，对每列跨行累加，输出 1 × nCols。
//
// 2. 如果矩阵列数 == Number：
//      认为每一列是一帧 / 一个脉冲，对每行跨列累加，输出 nRows × 1。
//
// 3. 如果行数能被 Number 整除：
//      认为输入按脉冲在行方向分块堆叠，输出 nRows / Number × nCols。
//
// 4. 如果列数能被 Number 整除：
//      认为输入按脉冲在列方向分块堆叠，输出 nRows × nCols / Number。
//
// 以上几种情况都等价于标量版：
//      对每个采样位置，跨 Number 个脉冲取 abs 后累加。
//------------------------------------------------------------------------------
bool RADAR_NonCoIntgr_M::Run()
{
	if (Number <= 0)
	{
		return false;
	}

	const SystemVueModelBuilder::Matrix< std::complex<double> >& inMat = input[0];
	SystemVueModelBuilder::Matrix< double >& outMat = output[0];

	const int nRows = static_cast<int>(inMat.NumRows());
	const int nCols = static_cast<int>(inMat.NumColumns());

	if (nRows <= 0 || nCols <= 0)
	{
		outMat.Resize(0, 0);
		return true;
	}

	// 情况 1：行数等于 Number，认为“行方向”为脉冲维
	if (nRows == Number)
	{
		return integratePulseByRows(inMat, outMat, nRows, nCols);
	}

	// 情况 2：列数等于 Number，认为“列方向”为脉冲维
	if (nCols == Number)
	{
		return integratePulseByCols(inMat, outMat, nRows, nCols);
	}

	// 情况 3：行方向可以按 Number 分块
	if ((nRows % Number) == 0)
	{
		return integrateBlockByRows(inMat, outMat, nRows, nCols);
	}

	// 情况 4：列方向可以按 Number 分块
	if ((nCols % Number) == 0)
	{
		return integrateBlockByCols(inMat, outMat, nRows, nCols);
	}

	// 如果矩阵尺寸无法与 Number 对应，说明输入矩阵并不是一个完整的非相干积分数据块
	return false;
}


//------------------------------------------------------------------------------
// 行方向为脉冲维：
// 输入： Number × nCols
// 输出： 1 × nCols
//------------------------------------------------------------------------------
bool RADAR_NonCoIntgr_M::integratePulseByRows(
	const SystemVueModelBuilder::Matrix< std::complex<double> >& inMat,
	SystemVueModelBuilder::Matrix< double >& outMat,
	int nRows,
	int nCols)
{
	outMat.Resize(1, nCols);

	for (int col = 0; col < nCols; ++col)
	{
		double sumAbs = 0.0;

		for (int row = 0; row < nRows; ++row)
		{
			sumAbs += std::abs(inMat(row, col));
		}

		outMat(0, col) = sumAbs;
	}

	return true;
}


//------------------------------------------------------------------------------
// 列方向为脉冲维：
// 输入： nRows × Number
// 输出： nRows × 1
//------------------------------------------------------------------------------
bool RADAR_NonCoIntgr_M::integratePulseByCols(
	const SystemVueModelBuilder::Matrix< std::complex<double> >& inMat,
	SystemVueModelBuilder::Matrix< double >& outMat,
	int nRows,
	int nCols)
{
	outMat.Resize(nRows, 1);

	for (int row = 0; row < nRows; ++row)
	{
		double sumAbs = 0.0;

		for (int col = 0; col < nCols; ++col)
		{
			sumAbs += std::abs(inMat(row, col));
		}

		outMat(row, 0) = sumAbs;
	}

	return true;
}


//------------------------------------------------------------------------------
// 行方向分块：
// 输入： Number * samplesPerPulseRows × nCols
// 输出： samplesPerPulseRows × nCols
//
// 适用于输入被按如下形式堆叠：
//      pulse0 的所有行
//      pulse1 的所有行
//      ...
//      pulseN 的所有行
//------------------------------------------------------------------------------
bool RADAR_NonCoIntgr_M::integrateBlockByRows(
	const SystemVueModelBuilder::Matrix< std::complex<double> >& inMat,
	SystemVueModelBuilder::Matrix< double >& outMat,
	int nRows,
	int nCols)
{
	const int outRows = nRows / Number;

	outMat.Resize(outRows, nCols);

	for (int row = 0; row < outRows; ++row)
	{
		for (int col = 0; col < nCols; ++col)
		{
			double sumAbs = 0.0;

			for (int pulse = 0; pulse < Number; ++pulse)
			{
				const int inRow = pulse * outRows + row;
				sumAbs += std::abs(inMat(inRow, col));
			}

			outMat(row, col) = sumAbs;
		}
	}

	return true;
}


//------------------------------------------------------------------------------
// 列方向分块：
// 输入： nRows × Number * samplesPerPulseCols
// 输出： nRows × samplesPerPulseCols
//
// 适用于输入被按如下形式堆叠：
//      pulse0 的所有列，pulse1 的所有列，...，pulseN 的所有列
//------------------------------------------------------------------------------
bool RADAR_NonCoIntgr_M::integrateBlockByCols(
	const SystemVueModelBuilder::Matrix< std::complex<double> >& inMat,
	SystemVueModelBuilder::Matrix< double >& outMat,
	int nRows,
	int nCols)
{
	const int outCols = nCols / Number;

	outMat.Resize(nRows, outCols);

	for (int row = 0; row < nRows; ++row)
	{
		for (int col = 0; col < outCols; ++col)
		{
			double sumAbs = 0.0;

			for (int pulse = 0; pulse < Number; ++pulse)
			{
				const int inCol = pulse * outCols + col;
				sumAbs += std::abs(inMat(row, inCol));
			}

			outMat(row, col) = sumAbs;
		}
	}

	return true;
}