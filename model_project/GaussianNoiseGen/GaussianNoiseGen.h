#pragma once
#include "SystemVue.h"
#include "ModelBuilder.h"
#include "TimedDFModel.h"
#include "TimedCircularBuffer.h"
#include <string>

class SYSTEMVUEMODELBUILDER_API GaussianNoiseGen : public SystemVueModelBuilder::TimedDFModel
{
public:
    enum SelectedShowAdvancedParams { No, Yes };
    enum SelectedSampleRateOption { UnTimed, TimedFromSampleRate, TimedFromSchematic };
    enum SelectedBurstMode { OFF, Single, Multiple };

    DECLARE_MODEL_INTERFACE(GaussianNoiseGen);

    GaussianNoiseGen();

    bool Setup() override;
    bool Run()   override;

    SystemVueModelBuilder::TimedCircularBuffer<double> output;

    double NDensity;
    double RefR;
    SelectedShowAdvancedParams ShowAdvancedParams;
    SelectedSampleRateOption SampleRateOption;
    double SampleRate;
    int InitialDelay;
    SelectedBurstMode BurstMode;
    int BurstLength;
    int BurstPeriod;
    int BurstDelay;
};
