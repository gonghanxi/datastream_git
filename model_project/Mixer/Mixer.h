#pragma once

#include "ModelBuilder.h"
#include "TimedDFModel.h"
#include "EnvelopeSignal.h"
#include "TimedCircularBuffer.h"
#include "SystemVue.h"
#include <complex>
#include <cstddef>

class SYSTEMVUEMODELBUILDER_API Mixer : public SystemVueModelBuilder::TimedDFModel {
public:
    enum EnableNoiseEnum
    {
        NO,
        YES
    };

    enum SidebandEnum
    {
        Lower,
        Upper
    };

    DECLARE_MODEL_INTERFACE(Mixer);
    Mixer();
    virtual bool Setup();
    virtual bool Run();

    ERESULT PropagateCharacterizationFrequency();

    SystemVueModelBuilder::EnvelopeCircularBuffer inPort;
    SystemVueModelBuilder::EnvelopeCircularBuffer loPort;
    SystemVueModelBuilder::EnvelopeCircularBuffer outPort;

    double ConvGain;
    EnableNoiseEnum EnableNoise;
    double NoiseFigure;
    SidebandEnum Sideband;
    double SidebandSuppression;
    double RfRej;
    double LoRej;
    double LoRfIso;
    double RfLoIso;
    double SOIout;
    double TOIout;
    double RefR;

    double fc_inPort;
    double fc_outPort;
    double fc_loPort;
};
