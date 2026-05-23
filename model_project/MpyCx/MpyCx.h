#pragma once
#include "ModelBuilder.h"
#include "DFModel.h"
#include "SystemVue.h"
#include <complex>
#include <cstddef>

class SYSTEMVUEMODELBUILDER_API MpyCx : public SystemVueModelBuilder::DFModel {
public:
	using cdouble = std::complex<double>;

	DECLARE_MODEL_INTERFACE(MpyCx);
	MpyCx();
	bool Setup() override;
	bool Run()   override;

	SystemVueModelBuilder::CircularBufferBusT<
		SystemVueModelBuilder::CircularBuffer<cdouble>
	> input;

	SystemVueModelBuilder::CircularBuffer<cdouble> output;
};
