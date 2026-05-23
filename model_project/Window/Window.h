#pragma once
#include "ModelBuilder.h"
#include "TimedDFModel.h"
#include "TimedCircularBuffer.h"
#include "SystemVue.h"

class SYSTEMVUEMODELBUILDER_API Window : public SystemVueModelBuilder::TimedDFModel
{
public:
    enum SelectedWindowType { Rectangle, Bartlett, Hanning, Hamming, Blackman, SteepBlackman, Kaiser };
    enum SelectedShowAdvancedParams { No, Yes };
    enum SelectedSampleRateOption { UnTimed, TimedFromSampleRate, TimedFromSchematic };

    DECLARE_MODEL_INTERFACE(Window);

    Window();

    bool Setup() override;
    bool Run() override;

    int factorial(int n);
    double I0(int n, double x);

    SystemVueModelBuilder::TimedCircularBuffer<double> output;

    SelectedWindowType WindowType;
    int Length;
    int ZeroPad;
    double KaiserParameter;
    SelectedShowAdvancedParams ShowAdvancedParams;
    SelectedSampleRateOption SampleRateOption;
    double SampleRate;
    int InitialDelay;
};
