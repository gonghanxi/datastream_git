#pragma once
#include "SystemVue.h"
#include "ModelBuilder.h"
#include <complex>

class SYSTEMVUEMODELBUILDER_API RADAR_Detector : public SystemVueModelBuilder::DFModel
{
public:
    enum SelectedDetectorType { Envelop, Square, LogSquare, Log };

    DECLARE_MODEL_INTERFACE(RADAR_Detector);

    RADAR_Detector();

    bool Run() override;

    SystemVueModelBuilder::CircularBuffer<std::complex<double>> input;
    SystemVueModelBuilder::CircularBuffer<double> output;

    SelectedDetectorType DetectorType;
    double Log_Coefb;
    double Log_Coefa;
};
