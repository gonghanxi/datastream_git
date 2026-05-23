#pragma once
#include "SystemVue.h"
#include "ModelBuilder.h"
#include "TimedDFModel.h"
#include "TimedCircularBuffer.h"

class SYSTEMVUEMODELBUILDER_API SineSweepGen : public SystemVueModelBuilder::TimedDFModel
{
public:
    enum SelectedFSweepType{ linear, log };
    enum SelectedShowAdvancedParams { No, Yes };
    enum SelectedSampleRateOption { UnTimed, TimedFromSampleRate, TimedFromSchematic };

    DECLARE_MODEL_INTERFACE(SineSweepGen);

    SineSweepGen();

    bool Setup() override;
    bool Run()   override;

    SystemVueModelBuilder::TimedCircularBuffer<double> output, frequency;

    double Amplitude;
    double Offset;
    SelectedFSweepType FSweepType;
    double StartFreq;
    double StopFreq;
    double Phase;
    double SweepPeriod;
    SelectedShowAdvancedParams ShowAdvancedParams;
    SelectedSampleRateOption SampleRateOption;
    double SampleRate;
    double InitialDelay;
};
