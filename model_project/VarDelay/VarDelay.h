#pragma once

#include "ModelBuilder.h"
#include "CircularBuffer.h"

class SYSTEMVUEMODELBUILDER_API VarDelay : public SystemVueModelBuilder::DFModel
{
public:
	DECLARE_MODEL_INTERFACE(VarDelay);

	VarDelay();

	virtual bool Setup() override;
	virtual bool Run()   override;

public:

	SystemVueModelBuilder::CircularBuffer<double> input;

	SystemVueModelBuilder::CircularBuffer<int>    control;

	SystemVueModelBuilder::CircularBuffer<double> output;

public:
	int MaxDelay;

private:
	SystemVueModelBuilder::CircularBuffer<double> m_buffer;

	size_t m_iDelay;
	size_t m_iMaxDelay;
};
