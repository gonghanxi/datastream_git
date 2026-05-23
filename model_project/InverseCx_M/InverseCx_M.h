#pragma once

#include "ModelBuilder.h"
#include "Matrix.h"
#include "MatrixMathFunction.h"

#include <complex>

class SYSTEMVUEMODELBUILDER_API InverseCx_M : public SystemVueModelBuilder::DFModel
{
public:
	DECLARE_MODEL_INTERFACE(InverseCx_M);

	InverseCx_M() = default;

	virtual bool Run() override;

	SystemVueModelBuilder::CircularBuffer<SystemVueModelBuilder::DComplexMatrix> input;
	SystemVueModelBuilder::CircularBuffer<SystemVueModelBuilder::DComplexMatrix> output;
};
