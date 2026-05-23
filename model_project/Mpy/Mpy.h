#pragma once
#include "ModelBuilder.h"
#include "DFModel.h"
#include <cstddef>
#include "SystemVue.h"

class SYSTEMVUEMODELBUILDER_API Mpy : public SystemVueModelBuilder::DFModel {
public:
	DECLARE_MODEL_INTERFACE(Mpy);
	Mpy();
	bool Setup() override;
	bool Run() override;

	SystemVueModelBuilder::CircularBufferBusT<
		SystemVueModelBuilder::CircularBuffer<double>
	> input;

	SystemVueModelBuilder::CircularBuffer<double> output;
};
