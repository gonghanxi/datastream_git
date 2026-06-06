#pragma once

#include "ModelBuilder.h"
#include "Matrix.h"

#include <complex>
#include <cmath>

// RADAR_NonCoIntgr_M：Signal non-coherent Integration for Matrix signals
// 输入：complex matrix
// 输出：real matrix
class SYSTEMVUEMODELBUILDER_API RADAR_NonCoIntgr_M : public SystemVueModelBuilder::DFModel
{
public:
	// SystemVue 模型接口宏
	DECLARE_MODEL_INTERFACE(RADAR_NonCoIntgr_M);

	// 构造函数
	RADAR_NonCoIntgr_M();

	// 系统函数
	virtual bool Setup();
	virtual bool Run();

	// ===== 端口 =====
	// input  : The input signal，complex matrix
	// output : The output signal after non-coherent integration，real matrix
	SystemVueModelBuilder::CircularBuffer< SystemVueModelBuilder::Matrix< std::complex<double> > > input;
	SystemVueModelBuilder::CircularBuffer< SystemVueModelBuilder::Matrix< double > > output;

	// ===== 参数 =====
	// 帮助文档中矩阵版只保留 Number 参数
	int Number;     // Number of Pulses for non-coherent integration，默认 5

private:
	// 按“行方向为脉冲维”做非相干积分
	bool integratePulseByRows(
		const SystemVueModelBuilder::Matrix< std::complex<double> >& inMat,
		SystemVueModelBuilder::Matrix< double >& outMat,
		int nRows,
		int nCols);

	// 按“列方向为脉冲维”做非相干积分
	bool integratePulseByCols(
		const SystemVueModelBuilder::Matrix< std::complex<double> >& inMat,
		SystemVueModelBuilder::Matrix< double >& outMat,
		int nRows,
		int nCols);

	// 按“行方向分块”做非相干积分，用于列向量或多行矩阵被按脉冲顺序堆叠的情况
	bool integrateBlockByRows(
		const SystemVueModelBuilder::Matrix< std::complex<double> >& inMat,
		SystemVueModelBuilder::Matrix< double >& outMat,
		int nRows,
		int nCols);

	// 按“列方向分块”做非相干积分，用于行向量或多列矩阵被按脉冲顺序堆叠的情况
	bool integrateBlockByCols(
		const SystemVueModelBuilder::Matrix< std::complex<double> >& inMat,
		SystemVueModelBuilder::Matrix< double >& outMat,
		int nRows,
		int nCols);
};
