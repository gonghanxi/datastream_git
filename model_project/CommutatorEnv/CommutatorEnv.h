#pragma once
#include "ModelBuilder.h"
#include "EnvelopeSignal.h"
#include "SystemVue.h"

class SYSTEMVUEMODELBUILDER_API CommutatorEnv : public SystemVueModelBuilder::DFModel
{
public:
	using BufferType = SystemVueModelBuilder::EnvelopeCircularBuffer;
	using BusType = SystemVueModelBuilder::EnvelopeCircularBufferBus;

	CommutatorEnv();

	bool Setup() override;
	bool Run() override;

	BusType    input;
	BufferType output;

	int    BlockSize;
	size_t m_iBlockSize;

	DECLARE_MODEL_INTERFACE(CommutatorEnv);
};
