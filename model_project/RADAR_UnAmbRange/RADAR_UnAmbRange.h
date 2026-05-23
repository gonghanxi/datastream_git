#pragma once
#include "SystemVue.h"
#include "ModelBuilder.h"

//-----------------------------------------------------------------------------------
// 目前只支持两组PRI的解模糊
//-----------------------------------------------------------------------------------

class SYSTEMVUEMODELBUILDER_API RADAR_UnAmbRange : public SystemVueModelBuilder::DFModel
{
public:
    // This Macro is required for all classes derived from DFModel
    DECLARE_MODEL_INTERFACE(RADAR_UnAmbRange);

    // Constructor to initialize parameters
    RADAR_UnAmbRange();

    //-------- Function Overloads --------
    virtual bool Run();

    // Ports
    SystemVueModelBuilder::IntCircularBufferBus Index;
    SystemVueModelBuilder::CircularBuffer<double> Range;

    // Parameter
    SystemVueModelBuilder::Matrix<double> PRI;
    int CPI_Num;
    double SampleRate;
};
