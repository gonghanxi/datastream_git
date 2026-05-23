#pragma once
#include "ModelBuilder.h"
#include "TimedDFModel.h"
#include "TimedCircularBuffer.h"
#include "SystemVue.h"

class SYSTEMVUEMODELBUILDER_API IID_Uniform : public SystemVueModelBuilder::TimedDFModel
{
public:
    enum SelectedShowAdvancedParams { No, Yes };
    enum SelectedSampleRateOption { UnTimed, TimedFromSampleRate, TimedFromSchematic };
    enum SelectedBurstMode { OFF, Single, Multiple };

    DECLARE_MODEL_INTERFACE(IID_Uniform);

    IID_Uniform();

    bool Setup() override;
    bool Run() override;

    SystemVueModelBuilder::TimedCircularBuffer<double> output;

    double LoLevel;
    double HiLevel;
    SelectedShowAdvancedParams ShowAdvancedParams;
    SelectedSampleRateOption SampleRateOption;
    double SampleRate;
    int InitialDelay;
    SelectedBurstMode BurstMode;
    int BurstLength;
    int BurstPeriod;
    int BurstDelay;
};
