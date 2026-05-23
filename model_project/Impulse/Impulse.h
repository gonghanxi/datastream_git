#pragma once
#include "SystemVue.h"
#include "ModelBuilder.h"
#include "TimedDFModel.h"
#include "TimedCircularBuffer.h"

class SYSTEMVUEMODELBUILDER_API Impulse : public SystemVueModelBuilder::TimedDFModel
{
public:
    enum SelectedNoOrYes { No, Yes };
    enum SelectedSampleRateOption { UnTimed, TimedFromSampleRate, TimedFromSchematic };
    enum SelectedBurstMode { OFF, Single, Multiple };

    DECLARE_MODEL_INTERFACE(Impulse);

    Impulse();

    bool Setup() override;
    bool Run()   override;

    SystemVueModelBuilder::TimedCircularBuffer<double> output;

    double Level;
    SelectedNoOrYes ScaleBySampleRate;
    SelectedNoOrYes ShowAdvancedParams;
    SelectedSampleRateOption SampleRateOption;
    double SampleRate;
    double InitialDelay;
    SelectedBurstMode BurstMode;
    double BurstLength;
    double BurstPeriod;
    double BurstDelay;
};
