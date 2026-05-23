#pragma once
#include "ModelBuilder.h"
#include "TimedDFModel.h"
#include "TimedCircularBuffer.h"
#include <complex>
#include "SystemVue.h"

class SYSTEMVUEMODELBUILDER_API TimeDelayCx : public SystemVueModelBuilder::TimedDFModel
{
public:
	enum UnitEnum {
		Unit_Time = 0,
		Unit_TimeStep = 1
	};

	DECLARE_MODEL_INTERFACE(TimeDelayCx);

	TimeDelayCx();

	bool Setup() override;
	bool Run() override;
	ERESULT CalculateLatency() override;

	SystemVueModelBuilder::TimedCircularBuffer<std::complex<double>> input;
	SystemVueModelBuilder::TimedCircularBuffer<std::complex<double>> output;

	UnitEnum Unit;
	double   T;
	int      N;

private:
	double delaySeconds_;
};
