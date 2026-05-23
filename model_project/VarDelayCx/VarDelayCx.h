#pragma once
#include "ModelBuilder.h"
#include "CircularBuffer.h"
#include <complex>

class SYSTEMVUEMODELBUILDER_API VarDelayCx : public SystemVueModelBuilder::DFModel
{
public:
	DECLARE_MODEL_INTERFACE(VarDelayCx);

	VarDelayCx();

	bool Setup() override;
	bool Run()   override;

	SystemVueModelBuilder::CircularBuffer<std::complex<double>> input;

	SystemVueModelBuilder::CircularBuffer<std::complex<double>> output;

	SystemVueModelBuilder::CircularBuffer<int> control;

	int MaxDelay;

private:
	SystemVueModelBuilder::CircularBuffer<std::complex<double>> m_buffer;

	size_t m_iDelay;
	size_t m_iMaxDelay;
};
