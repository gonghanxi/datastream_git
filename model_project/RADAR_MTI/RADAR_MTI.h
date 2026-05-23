#pragma once
#include "SystemVue.h"
#include "ModelBuilder.h"
#include "Matrix.h"
#include "MatrixCircularBuffer.h"
#include "EnvelopeSignal.h"

class SYSTEMVUEMODELBUILDER_API RADAR_MTI : public SystemVueModelBuilder::DFModel
{
public:
    enum SelectedMTI_Type {
        TwoPulseCanceller,
        ThreePulseCanceller
    };

    DECLARE_MODEL_INTERFACE(RADAR_MTI);

    RADAR_MTI();

    bool Run() override;
    bool Setup() override;

    SystemVueModelBuilder::DComplexCircularBuffer input;
    SystemVueModelBuilder::DComplexCircularBuffer output;

    double PRI;
    double SampleRate;
    int NumOfPulse;
    SelectedMTI_Type MTI_Type;

private:
    int samplesPerPulse;
    int inputTotalSamples;
    int outputTotalSamples;
};
