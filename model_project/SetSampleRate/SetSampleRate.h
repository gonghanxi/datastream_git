#pragma once
#include "ModelBuilder.h"
#include "TimedDFModel.h"
#include "TimedCircularBuffer.h"
#include <cmath>
#include "SystemVue.h"

class SYSTEMVUEMODELBUILDER_API SetSampleRate : public SystemVueModelBuilder::TimedDFModel {
public:
	DECLARE_MODEL_INTERFACE(SetSampleRate);

	SetSampleRate();
	bool Setup() override;
	bool Run() override;

	SystemVueModelBuilder::TimedCircularBuffer<double> input;
	SystemVueModelBuilder::TimedCircularBuffer<double> output;

	double SampleRate;

private:
	static inline bool almost_equal(double a, double b, double eps = 1e-12) {
		const double ma = std::fabs(a), mb = std::fabs(b);
		const double scale = (ma > mb ? ma : mb);
		return std::fabs(a - b) <= eps * (scale > 1.0 ? scale : 1.0);
	}
};
