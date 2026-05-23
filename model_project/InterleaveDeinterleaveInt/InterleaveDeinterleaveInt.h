#pragma once

#include "ModelBuilder.h"
#include "CircularBuffer.h"

namespace SystemVueModelBuilder
{
	// 整数交织/解交织（等价矩阵转置）
	class InterleaveDeinterleaveInt : public DFModel
	{
	public:
		DECLARE_MODEL_INTERFACE(InterleaveDeinterleaveInt);

		InterleaveDeinterleaveInt();

		bool Setup() override;
		bool Initialize() override;
		bool Run() override;

	public:
		// 端口：整数序列（黄色 int）
		CircularBuffer<int> input;
		CircularBuffer<int> output;

		// 参数
		int Rows;
		int Columns;
	};
}
