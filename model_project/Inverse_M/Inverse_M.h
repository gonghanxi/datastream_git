#pragma once

#include "ModelBuilder.h"
#include "Matrix.h"
#include "MatrixMathFunction.h"
#include <cstddef>

class SYSTEMVUEMODELBUILDER_API Inverse_M : public SystemVueModelBuilder::DFModel
{
public:
	DECLARE_MODEL_INTERFACE(Inverse_M);

	Inverse_M();

	virtual bool Setup() override;
	virtual bool Run()   override;

	SystemVueModelBuilder::CircularBuffer<
		SystemVueModelBuilder::Matrix<double>
	> input;

	SystemVueModelBuilder::CircularBuffer<
		SystemVueModelBuilder::Matrix<double>
	> output;
};
