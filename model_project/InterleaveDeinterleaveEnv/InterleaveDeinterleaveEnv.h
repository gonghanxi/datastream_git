#pragma once
#pragma once

#include "ModelBuilder.h"
#include "EnvelopeSignal.h"

namespace SystemVueModelBuilder
{
	class InterleaveDeinterleaveEnv : public DFModel
	{
	public:
		DECLARE_MODEL_INTERFACE(InterleaveDeinterleaveEnv);

		InterleaveDeinterleaveEnv();

		bool Setup() override;
		bool Initialize() override;
		bool Run() override;

	public:
		EnvelopeCircularBuffer input;   // 输入（Envelope序列）
		EnvelopeCircularBuffer output;  // 输出（Envelope序列）

		int Rows;
		int Columns;

//	private:
		unsigned m_blockSize;
	};
}
