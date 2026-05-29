#pragma once
#include "ModelBuilder.h"
#include "DFModel.h"
#include "CircularBuffer.h"
#include "Matrix.h"
#include <complex>

class SYSTEMVUEMODELBUILDER_API Conjugate_M : public SystemVueModelBuilder::DFModel {
public:
	DECLARE_MODEL_INTERFACE(Conjugate_M);

	Conjugate_M();

	bool Setup() override;
	bool Run()   override;

	SystemVueModelBuilder::CircularBuffer<
		SystemVueModelBuilder::Matrix< std::complex<double> >
	> input;

	SystemVueModelBuilder::CircularBuffer<
		SystemVueModelBuilder::Matrix< std::complex<double> >
	> output;
};
