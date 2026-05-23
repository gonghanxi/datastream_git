#pragma once
#include "ModelBuilder.h"
#include "CircularBuffer.h"
#include <complex>
#include "SystemVue.h"

class SYSTEMVUEMODELBUILDER_API RADAR_MatchedFilter : public SystemVueModelBuilder::DFModel
{
public:
    DECLARE_MODEL_INTERFACE(RADAR_MatchedFilter);

    RADAR_MatchedFilter();

    bool Setup() override;
    bool Run() override;

    SystemVueModelBuilder::CircularBuffer<std::complex<double>> signal;
    SystemVueModelBuilder::CircularBuffer<std::complex<double>> reference;
    SystemVueModelBuilder::CircularBuffer<std::complex<double>> output;

    double PulseWidth;
    double PRI;
    double SampleRate;
};
