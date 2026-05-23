#pragma once
#include "ModelBuilder.h"
#include "DFModel.h"
#include "CircularBuffer.h"
#include <cstddef>
#include <cmath>
#include "SystemVue.h"

class SYSTEMVUEMODELBUILDER_API Sub : public SystemVueModelBuilder::DFModel {
public:
	DECLARE_MODEL_INTERFACE(Sub);

	Sub();
	bool Setup() override;
	bool Run() override;

	SystemVueModelBuilder::CircularBuffer<double> pos;
	SystemVueModelBuilder::CircularBufferBusT< SystemVueModelBuilder::CircularBuffer<double> > neg;
	SystemVueModelBuilder::CircularBuffer<double> output;
};
