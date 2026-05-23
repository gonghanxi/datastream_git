#pragma once
#include "SystemVue.h"
#include "ModelBuilder.h"
#include "TimedDFModel.h"
#include "TimedCircularBuffer.h"

class SYSTEMVUEMODELBUILDER_API ChirpGen : public SystemVueModelBuilder::TimedDFModel
{
public:
    enum ShowAdvancedParamsEnum { NO, YES };
    enum SampleRateOptionEnum { UnTimed, TimedfromSampleRate, TimedfromSchematic };

    DECLARE_MODEL_INTERFACE(ChirpGen);

    ChirpGen();

    bool Setup() override;
    bool Run() override;

    SystemVueModelBuilder::TimedCircularBuffer<double> freqOutput;
    SystemVueModelBuilder::TimedCircularBuffer<double> SigOutput;

    double Amplitude;
    double Offset;
    double StartFreq;
    double StopFreq;
    double Phase;
    double SweepPeriod;
    ShowAdvancedParamsEnum ShowAdvancedParams;
    SampleRateOptionEnum SampleRateOption;
    double SampleRate;
    double InitialDelay;

    int counter;
};
