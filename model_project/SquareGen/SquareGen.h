#pragma once
#include "ModelBuilder.h"
#include "TimedDFModel.h"
#include "TimedCircularBuffer.h"
#include "SystemVue.h"

class SYSTEMVUEMODELBUILDER_API SquareGen : public SystemVueModelBuilder::TimedDFModel
{
public:
    enum SelectedPolarity { normal, inverted };
    enum SelectedShowAdvancedParams { No, Yes };
    enum SelectedSampleRateOption { UnTimed, TimedFromSampleRate, TimedFromSchematic };
    enum SelectedBurstMode { OFF, Single, Multiple };

    DECLARE_MODEL_INTERFACE(SquareGen);

    SquareGen();

    bool Setup() override;
    bool Run() override;

    SystemVueModelBuilder::TimedCircularBuffer<double> output;

    double LoLevel;
    double HiLevel;
    double Frequency;
    double Phase;
    double DutyCycle;
    SelectedPolarity Polarity;
    SelectedShowAdvancedParams ShowAdvancedParams;
    SelectedSampleRateOption SampleRateOption;
    double SampleRate;
    double InitialDelay;
    SelectedBurstMode BurstMode;
    double BurstLength;
    double BurstPeriod;
    double BurstDelay;
};
