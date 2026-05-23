#pragma once

#include "ModelBuilder.h"
#include "EnvelopeSignal.h"
#include "CircularBuffer.h"

class SYSTEMVUEMODELBUILDER_API VarDelayEnv : public SystemVueModelBuilder::DFModel
{
public:
	DECLARE_MODEL_INTERFACE(VarDelayEnv);

	VarDelayEnv();

	virtual bool Setup() override;
	virtual bool Run()   override;

public:
	SystemVueModelBuilder::EnvelopeCircularBuffer input;
	SystemVueModelBuilder::CircularBuffer<int>    control;
	SystemVueModelBuilder::EnvelopeCircularBuffer output;

public:
	int MaxDelay;

private:
	SystemVueModelBuilder::EnvelopeCircularBuffer m_buffer;

	size_t m_iDelay;
	size_t m_iMaxDelay;
};

