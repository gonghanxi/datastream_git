#pragma once
#include "ModelBuilder.h"
#include "TimedDFModel.h"
#include "TimedCircularBuffer.h"
#include <complex>
#include "SystemVue.h"

class SYSTEMVUEMODELBUILDER_API ComplexExpGen : public SystemVueModelBuilder::TimedDFModel
{
public:
    enum SelectedQuadraturePolarity { normal, inverted };
    enum SelectedShowAdvancedParams { No, Yes };
    enum SelectedSampleRateOption { UnTimed, TimedFromSampleRate, TimedFromSchematic };
    enum SelectedBurstMode { OFF, Single, Multiple };

    DECLARE_MODEL_INTERFACE(ComplexExpGen);

    ComplexExpGen();

    bool Setup() override;
    bool Run() override;

    SystemVueModelBuilder::TimedCircularBuffer<std::complex<double>> output;

    double Amplitude;
    std::complex<double> Offset;
    double Frequency;
    double Phase;
    SelectedQuadraturePolarity QuadraturePolarity;
    SelectedShowAdvancedParams ShowAdvancedParams;
    SelectedSampleRateOption SampleRateOption;
    double SampleRate;
    double InitialDelay;
    SelectedBurstMode BurstMode;
    double BurstLength;
    double BurstPeriod;
    double BurstDelay;

    int QuadraturePolaritySign;
};
