#pragma once
#include "ModelBuilder.h"
#include "CircularBuffer.h"
#include "SystemVue.h"
#include <complex>


class SYSTEMVUEMODELBUILDER_API RADAR_CoIntgr : public SystemVueModelBuilder::DFModel
{
public:
    DECLARE_MODEL_INTERFACE(RADAR_CoIntgr);

    RADAR_CoIntgr();

    bool Setup() override;
    bool Run() override;

    SystemVueModelBuilder::CircularBuffer<std::complex<double>> input, output;

    double PRI_Or_WaveGate;
    int NumOfPulse;
    double SampleRate;
};
