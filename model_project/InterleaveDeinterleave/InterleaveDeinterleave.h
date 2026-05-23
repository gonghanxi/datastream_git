#pragma once

#include "ModelBuilder.h"
#include "CircularBuffer.h"

namespace SystemVueModelBuilder
{
	//============================================================
	// InterleaveDeinterleave（float版）
	// 功能：块交织/解交织（等价于矩阵转置）
	// 行为：每次读取 N=Rows*Columns 个样本，按行写入矩阵，再按列读出输出
	//============================================================
	class InterleaveDeinterleave : public DFModel
	{
	public:
		// 必须：声明模型接口
		DECLARE_MODEL_INTERFACE(InterleaveDeinterleave);

		InterleaveDeinterleave();

		bool Setup() override;
		bool Initialize() override;
		bool Run() override;

	public:
		//-------------------- 端口 --------------------
		CircularBuffer<float> input;   // 输入（float序列）
		CircularBuffer<float> output;  // 输出（float序列）

		//-------------------- 参数 --------------------
		int Rows;     // Number of rows of the interleave/deinterleave matrix
		int Columns;  // Number of columns of the interleave/deinterleave matrix

//	private:
		// 内部：块长度
		unsigned m_blockSize;
	};
}
