#pragma once
#include "ModelBuilder.h"
#include "TimedDFModel.h"
#include "TimedCircularBuffer.h"
#include "SystemVue.h"

class SYSTEMVUEMODELBUILDER_API IID_Gaussian : public SystemVueModelBuilder::TimedDFModel
{
public:
    enum SelectedShowAdvancedParams { No, Yes };
    enum SelectedSampleRateOption { UnTimed, TimedFromSampleRate, TimedFromSchematic };
    enum SelectedBurstMode { OFF, Single, Multiple };

    DECLARE_MODEL_INTERFACE(IID_Gaussian);

    IID_Gaussian();

    bool Setup() override;
    bool Run() override;

    SystemVueModelBuilder::TimedCircularBuffer<double> output;

    double StdDev;
    double Offset;
    SelectedShowAdvancedParams ShowAdvancedParams;
    SelectedSampleRateOption SampleRateOption;
    double SampleRate;
    int InitialDelay;
    SelectedBurstMode BurstMode;
    int BurstLength;
    int BurstPeriod;
    int BurstDelay;
};
