#pragma once

#include "ModelBuilder.h"
#include "Matrix.h"

class SYSTEMVUEMODELBUILDER_API LookUpTable : public SystemVueModelBuilder::DFModel
{
public:
	DECLARE_MODEL_INTERFACE(LookUpTable);

	LookUpTable();

	virtual bool Setup() override;
	virtual bool Run()   override;

	SystemVueModelBuilder::IntCircularBuffer          input;
	SystemVueModelBuilder::CircularBuffer<double>     output;

	SystemVueModelBuilder::DoubleMatrix               Values;

private:
	std::size_t numDims_;      
	std::size_t firstDim_;     
	std::size_t numElements_;  
};
