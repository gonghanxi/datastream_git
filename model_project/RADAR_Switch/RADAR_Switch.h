#pragma once
#include "SystemVue.h"
#include "ModelBuilder.h"
#include "EnvelopeSignal.h"
#include "TimedDFModel.h"

class SYSTEMVUEMODELBUILDER_API RADAR_Switch : public SystemVueModelBuilder::TimedDFModel
{
public:
    DECLARE_MODEL_INTERFACE(RADAR_Switch);

    RADAR_Switch();

    virtual bool Run();

    SystemVueModelBuilder::EnvelopeCircularBuffer input, output;
    SystemVueModelBuilder::CircularBuffer<double> PRI;

    double PRF;
    double SwitchOff_Time;
};
