#pragma once
#include "ModelBuilder.h"
#include "EnvelopeSignal.h"
#include <sstream>
#include <cmath>
#include <algorithm>
#include "SystemVue.h"

class SYSTEMVUEMODELBUILDER_API AsyncCommutatorEnv : public SystemVueModelBuilder::DFModel
{
public:
	using BufferType = SystemVueModelBuilder::EnvelopeCircularBuffer;
	using BusType = SystemVueModelBuilder::EnvelopeCircularBufferBus;

	AsyncCommutatorEnv();

	bool Setup() override;
	bool Run() override;

	BusType    input;
	BufferType output;

	SystemVueModelBuilder::Matrix<int> BlockSizes;

	DECLARE_MODEL_INTERFACE(AsyncCommutatorEnv);

private:
	double m_fcOut;
};
