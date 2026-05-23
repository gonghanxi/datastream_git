#pragma once
#include "ModelBuilder.h"
#include "DFModel.h"
#include <complex>
#include "SystemVue.h"

class SYSTEMVUEMODELBUILDER_API AddCx : public SystemVueModelBuilder::DFModel {
public:
	using cdouble = std::complex<double>;

	DECLARE_MODEL_INTERFACE(AddCx);
	AddCx();
	bool Setup() override;
	bool Run()   override;

	SystemVueModelBuilder::CircularBufferBusT<
		SystemVueModelBuilder::CircularBuffer<cdouble>
	> input;
	SystemVueModelBuilder::CircularBuffer<cdouble> output;
};
