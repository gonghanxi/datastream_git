#pragma once
#include "ModelBuilder.h"
#include "TimedDFModel.h"
#include "TimedCircularBuffer.h"
#include "SystemVue.h"

class SYSTEMVUEMODELBUILDER_API TimeDelay : public SystemVueModelBuilder::TimedDFModel
{
public:
    enum UnitEnum {
        Unit_Time = 0,
        Unit_TimeStep = 1
    };

    DECLARE_MODEL_INTERFACE(TimeDelay);

    TimeDelay();

    bool Setup() override;
    bool Run() override;
    ERESULT CalculateLatency() override;

    SystemVueModelBuilder::TimedCircularBuffer<double> input;
    SystemVueModelBuilder::TimedCircularBuffer<double> output;

    UnitEnum Unit;
    double T;
    int N;

private:
    double delaySeconds_;
};
