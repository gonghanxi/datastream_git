#pragma once
#include "ModelBuilder.h"
#include "TimedDFModel.h"
#include "TimedCircularBuffer.h"
#include "SystemVue.h"

class SYSTEMVUEMODELBUILDER_API TimeDelayInt : public SystemVueModelBuilder::TimedDFModel
{
public:
	enum UnitEnum {
		Unit_Time = 0,
		Unit_TimeStep = 1
	};

	DECLARE_MODEL_INTERFACE(TimeDelayInt);

	TimeDelayInt();

	bool Setup() override;
	bool Run() override;
	ERESULT CalculateLatency() override;

	SystemVueModelBuilder::TimedCircularBuffer<int> input;
	SystemVueModelBuilder::TimedCircularBuffer<int> output;

	UnitEnum Unit;
	double   T;
	int      N;

private:
	double delaySeconds_;
};
