#pragma once

#include "ModelBuilder.h"
#include "Matrix.h"
#include "MatrixMathFunction.h"

#include <complex>

class SYSTEMVUEMODELBUILDER_API InverseCx_M : public SystemVueModelBuilder::DFModel
{
public:
	DECLARE_MODEL_INTERFACE(InverseCx_M);

    InverseCx_M();

	virtual bool Run() override;
    virtual bool Setup() override;

    SystemVueModelBuilder::CircularBuffer< SystemVueModelBuilder::Matrix<std::complex<double>> > input;
    SystemVueModelBuilder::CircularBuffer< SystemVueModelBuilder::Matrix<std::complex<double>> > output;
};
