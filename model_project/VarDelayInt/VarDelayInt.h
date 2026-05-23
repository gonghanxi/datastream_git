#pragma once
#include "ModelBuilder.h"
#include "CircularBuffer.h"

class SYSTEMVUEMODELBUILDER_API VarDelayInt : public SystemVueModelBuilder::DFModel
{
public:
	DECLARE_MODEL_INTERFACE(VarDelayInt);

	VarDelayInt();

	bool Setup() override;
	bool Run()   override;

	SystemVueModelBuilder::CircularBuffer<int> input;

	SystemVueModelBuilder::CircularBuffer<int> output;

	SystemVueModelBuilder::CircularBuffer<int> control;

	int MaxDelay;   

private:
	SystemVueModelBuilder::CircularBuffer<int> m_buffer;

	size_t m_iDelay;
	size_t m_iMaxDelay;
};
