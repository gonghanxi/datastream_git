#pragma once
#include "ModelBuilder.h"
#include "DFModel.h"
#include "TimedCircularBuffer.h"
#include <deque>
#include <vector>
#include <algorithm>
#include <cmath>

class TimeSynchronizer : public SystemVueModelBuilder::DFModel {
public:
	enum ModeEnum { ZeroPadding = 0, TimeDelay = 1 };

	DECLARE_MODEL_INTERFACE(TimeSynchronizer);
	TimeSynchronizer();

	bool Setup() override;
	bool Run()   override;

	SystemVueModelBuilder::CircularBufferBusT<
		SystemVueModelBuilder::TimedCircularBuffer<double>
	> input;

	SystemVueModelBuilder::CircularBufferBusT<
		SystemVueModelBuilder::TimedCircularBuffer<double>
	> output;

	ModeEnum Mode;

private:
	struct SampleD { double v; double t; };

	std::vector<std::deque<SampleD>> fifos_;
	std::vector<double> lastValue_;
	int N_ = 0;

	static inline double eps() { return 1e-15; }
};
