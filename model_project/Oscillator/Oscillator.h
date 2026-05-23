#pragma once
#include "ModelBuilder.h"
#include "EnvelopeSignal.h"
#include "TimedDFModel.h"
#include "SystemVue.h"

class SYSTEMVUEMODELBUILDER_API Oscillator : public SystemVueModelBuilder::TimedDFModel
{
public:
    enum SelectedYesOrNo { No, Yes };
    enum SelectedPN_Type { RandomPN, FixedFreqOffset, FixedFreqOffsetAndAmplitude };
    enum SelectedSampleRateOption { UnTimed, TimedFromSampleRate, TimedFromSchematic };

    DECLARE_MODEL_INTERFACE(Oscillator);

    Oscillator();

    ERESULT PropagateCharacterizationFrequency();
    bool Setup() override;
    bool Run() override;

    SystemVueModelBuilder::EnvelopeCircularBuffer output;

    double Frequency;
    double Power;
    double Phase;
    SelectedYesOrNo RandomPhase;
    double NDensity;
    double RefR;
    SelectedYesOrNo ShowAdvancedParams;
    SelectedSampleRateOption SampleRateOption;
    double SampleRate;
    double InitialDelay;
};
