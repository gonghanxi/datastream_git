#pragma once

#include "ModelBuilder.h"
#include "DFModel.h"
#include "CircularBuffer.h"
#include <cmath>
#include <cstddef>

class SYSTEMVUEMODELBUILDER_API AdaptLinQuant : public SystemVueModelBuilder::DFModel
{
public:
	DECLARE_MODEL_INTERFACE(AdaptLinQuant);

	AdaptLinQuant();

	virtual bool Setup() override;
	virtual bool Run()   override;

	SystemVueModelBuilder::CircularBuffer<double> input;    
	SystemVueModelBuilder::CircularBuffer<double> inStep;   

	SystemVueModelBuilder::CircularBuffer<double> amplitude; 
	SystemVueModelBuilder::CircularBuffer<double> outStep;   
	SystemVueModelBuilder::CircularBuffer<int>    stepLevel; 

	int Bits;   
};
