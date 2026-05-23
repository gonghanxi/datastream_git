#pragma once
#include "SystemVue.h"
#include "ModelBuilder.h"

//-----------------------------------------------------------------------------------
// 目前只支持两组PRI的解模糊
//-----------------------------------------------------------------------------------

class SYSTEMVUEMODELBUILDER_API RADAR_UnAmbVelocity : public SystemVueModelBuilder::DFModel
{
public:
    enum SelectedDirection { ApproachingRadar, LeavingRadar };

    // This Macro is required for all classes derived from DFModel
    DECLARE_MODEL_INTERFACE(RADAR_UnAmbVelocity);

    // Constructor to initialize parameters
    RADAR_UnAmbVelocity();

    //-------- Function Overloads --------
    virtual bool Run();

    // Ports
    SystemVueModelBuilder::IntCircularBufferBus Index;
    SystemVueModelBuilder::CircularBuffer<double> Velocity;

    // Parameter
    SystemVueModelBuilder::Matrix<double> PRI;
    int CPI_Num;
    double fc;
    double SampleRate;
    SelectedDirection Direction;
};
