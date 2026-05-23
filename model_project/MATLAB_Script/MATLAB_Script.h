#pragma once
#include "ModelBuilder.h"
#include "DFModel.h"
#include <complex>

class SYSTEMVUEMODELBUILDER_API MATLAB_Script : public SystemVueModelBuilder::DFModel {
public:
	using cdouble = std::complex<double>;

    DECLARE_MODEL_INTERFACE(MATLAB_Script);
    MATLAB_Script();
	bool Setup() override;
	bool Run()   override;

	SystemVueModelBuilder::CircularBufferBusT<
		SystemVueModelBuilder::CircularBuffer<cdouble>
	> input;
	SystemVueModelBuilder::CircularBuffer<cdouble> output;

};
