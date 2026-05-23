#pragma once
#include "ModelBuilder.h"
#include "TimedDFModel.h"
#include "EnvelopeSignal.h"
#include "SystemVue.h"

class SYSTEMVUEMODELBUILDER_API TimeDelayEnv : public SystemVueModelBuilder::TimedDFModel
{
public:
	enum UnitEnum {
		Unit_Time = 0,
		Unit_TimeStep = 1
	};

	DECLARE_MODEL_INTERFACE(TimeDelayEnv);

	TimeDelayEnv();

	bool Setup() override;
	bool Run() override;
	ERESULT CalculateLatency() override;

	SystemVueModelBuilder::EnvelopeCircularBuffer input;
	SystemVueModelBuilder::EnvelopeCircularBuffer output;

	UnitEnum Unit;
	double   T;
	int      N;

private:
	double delaySeconds_;
};
