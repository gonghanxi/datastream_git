#pragma once
#include "ModelBuilder.h"
#include "TimedDFModel.h"
#include "TimedCircularBuffer.h"
#include "SystemVue.h"

class SYSTEMVUEMODELBUILDER_API SineGen : public SystemVueModelBuilder::TimedDFModel
{
public:
    enum SelectedShowAdvancedParams { No, Yes };
    enum SelectedSampleRateOption { UnTimed, TimedFromSampleRate, TimedFromSchematic };
    enum SelectedBurstMode { OFF, Single, Multiple };

    DECLARE_MODEL_INTERFACE(SineGen);

    SineGen();

    bool Setup() override;
    bool Run() override;

    SystemVueModelBuilder::TimedCircularBuffer<double> output;

    double Amplitude;
    double Offset;
    double Frequency;
    double Phase;
    SelectedShowAdvancedParams ShowAdvancedParams;
    SelectedSampleRateOption SampleRateOption;
    double SampleRate;
    double InitialDelay;
    SelectedBurstMode BurstMode;
    double BurstLength;
    double BurstPeriod;
    double BurstDelay;
};
