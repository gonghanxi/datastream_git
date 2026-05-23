#pragma once
#include "SystemVue.h"
#include "ModelBuilder.h"
#include "TimedDFModel.h"
#include "TimedCircularBuffer.h"

class SYSTEMVUEMODELBUILDER_API SquareSweepGen : public SystemVueModelBuilder::TimedDFModel
{
public:
    enum SelectedFSweepType { linear, log };
    enum SelectedPolarity { normal, inverted };
    enum SelectedShowAdvancedParams { No, Yes };
    enum SelectedSampleRateOption { UnTimed, TimedFromSampleRate, TimedFromSchematic };

    DECLARE_MODEL_INTERFACE(SquareSweepGen);

    SquareSweepGen();

    bool Setup() override;
    bool Run()   override;

    SystemVueModelBuilder::TimedCircularBuffer<double> output, frequency;

    double LoLevel;
    double HiLevel;
    SelectedFSweepType FSweepType;
    double StartFreq;
    double StopFreq;
    double Phase;
    double SweepPeriod;
    double DutyCycle;
    SelectedPolarity Polarity;
    SelectedShowAdvancedParams ShowAdvancedParams;
    SelectedSampleRateOption SampleRateOption;
    double SampleRate;
    double InitialDelay;
};
