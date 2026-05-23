#pragma once

#include "ModelBuilder.h"
#include "SystemVue.h"

class SYSTEMVUEMODELBUILDER_API CommutatorInt : public SystemVueModelBuilder::DFModel
{
public:
	using BufferType = SystemVueModelBuilder::CircularBuffer<int>;
	using BusType = SystemVueModelBuilder::CircularBufferBusT<BufferType>;

	CommutatorInt();

	bool Setup() override;
	bool Run() override;

	BusType input;
	BufferType output;

	int BlockSize;
	size_t m_iBlockSize;

	DECLARE_MODEL_INTERFACE(CommutatorInt);
};
