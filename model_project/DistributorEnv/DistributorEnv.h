#pragma once
#pragma once

#include "ModelBuilder.h"
#include "DFModel.h"
#include "EnvelopeSignal.h"

namespace SystemVueModelBuilder
{
	using EnvStream = EnvelopeCircularBuffer;
	using EnvStreamBus = EnvelopeCircularBufferBus;

    class SYSTEMVUEMODELBUILDER_API DistributorEnv : public DFModel
	{
	public:
		DistributorEnv();

		EnvStream    input;
		EnvStreamBus output;

		int BlockSize;

		bool Setup() override;
		bool Run()   override;

		DECLARE_MODEL_INTERFACE(DistributorEnv);

	private:
		size_t m_iBlockSize;
	};
}
