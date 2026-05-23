#pragma once

#include "ModelBuilder.h"
#include "CircularBuffer.h"

#include <complex>

namespace SystemVueModelBuilder
{
	// 复数交织/解交织（等价矩阵转置）
	class InterleaveDeinterleaveCx : public DFModel
	{
	public:
		DECLARE_MODEL_INTERFACE(InterleaveDeinterleaveCx);

		InterleaveDeinterleaveCx();

		bool Setup() override;
		bool Initialize() override;
		bool Run() override;

	public:
		// 端口：复数序列（绿色 complex 通常对应 complex<double>）
		CircularBuffer<std::complex<double>> input;
		CircularBuffer<std::complex<double>> output;

		// 参数
		int Rows;     // Number of rows of the interleave/deinterleave matrix
		int Columns;  // Number of columns of the interleave/deinterleave matrix
	};
}
