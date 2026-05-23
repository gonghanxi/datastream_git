#pragma once
#include "ModelBuilder.h"
#include "DFModel.h"
#include "SystemVue.h"

class SYSTEMVUEMODELBUILDER_API Add : public SystemVueModelBuilder::DFModel {
public:
	DECLARE_MODEL_INTERFACE(Add);
	Add();
	bool Setup() override;
	bool Run() override;

	SystemVueModelBuilder::CircularBufferBusT<
		SystemVueModelBuilder::CircularBuffer<double>
	> input;
	SystemVueModelBuilder::CircularBuffer<double> output;
};
