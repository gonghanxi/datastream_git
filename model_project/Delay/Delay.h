#pragma once
#include "ModelBuilder.h"
#include "DFModel.h"
#include "CircularBuffer.h"
#include "SystemVue.h"
#include <vector>
#include <cstddef>

class SYSTEMVUEMODELBUILDER_API Delay : public SystemVueModelBuilder::DFModel {
public:
	enum OutputTimingEnum { EqualToInput = 0, BeforeInput = 1 };

	DECLARE_MODEL_INTERFACE(Delay);

	Delay();

	bool Setup() override;
	bool Run() override;

	SystemVueModelBuilder::CircularBuffer<double> input;
	SystemVueModelBuilder::CircularBuffer<double> output;

	int N;
	OutputTimingEnum OutputTiming;

private:
	std::vector<double> buf_;
	std::size_t head_;
	int warmup_;
};
