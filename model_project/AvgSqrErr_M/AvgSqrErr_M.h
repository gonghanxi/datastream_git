#pragma once

#include "ModelBuilder.h"
#include "DFModel.h"
#include "CircularBuffer.h"
#include "Matrix.h"

#include <cstddef>
#include <cmath>

class SYSTEMVUEMODELBUILDER_API AvgSqrErr_M : public SystemVueModelBuilder::DFModel
{
public:
	DECLARE_MODEL_INTERFACE(AvgSqrErr_M);

	AvgSqrErr_M();

	bool Setup() override;
	bool Run() override;

	SystemVueModelBuilder::CircularBuffer<
		SystemVueModelBuilder::Matrix<double>
	> input1;

	SystemVueModelBuilder::CircularBuffer<
		SystemVueModelBuilder::Matrix<double>
	> input2;

	SystemVueModelBuilder::CircularBuffer<double> output;

	int NumInputsToAverage;
};
