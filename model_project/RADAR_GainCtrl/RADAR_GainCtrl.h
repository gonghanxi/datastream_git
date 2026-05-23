#pragma once
#include "SystemVue.h"
#include "ModelBuilder.h"
#include "EnvelopeSignal.h"
#include "TimedDFModel.h"

class SYSTEMVUEMODELBUILDER_API RADAR_GainCtrl : public SystemVueModelBuilder::TimedDFModel
{
public:
    enum SelectedControlType { MGC, STC, AGC };

    DECLARE_MODEL_INTERFACE(RADAR_GainCtrl);

    RADAR_GainCtrl();

    virtual bool Run();

    SystemVueModelBuilder::EnvelopeCircularBuffer input, output;
    SystemVueModelBuilder::CircularBuffer<double> gain;

    SelectedControlType ControlType;
    double PRI;
    double Gain;
    double STC_Factor;
    double STC_StartTime;
    double STC_StopTime;
    double STC_K_Coef;
};
