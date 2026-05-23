#pragma once

#include "ModelBuilder.h"
#include "CircularBuffer.h"

#include <vector>

class SYSTEMVUEMODELBUILDER_API Convolve : public SystemVueModelBuilder::DFModel
{
public:
	DECLARE_MODEL_INTERFACE(Convolve);

	Convolve();

	virtual bool Setup() override;
	virtual bool Run()   override;

	SystemVueModelBuilder::CircularBuffer<double> inA;

	SystemVueModelBuilder::CircularBuffer<double> inB;

	SystemVueModelBuilder::CircularBuffer<double> out;

	int TruncationDepth;

private:
	std::vector<double> histA_;   
	std::vector<double> histB_;   

	unsigned long long iter_;
};
