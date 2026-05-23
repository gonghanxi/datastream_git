#pragma once
#include "SystemVue.h"
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API RADAR_VelocityMeas : public SystemVueModelBuilder::DFModel
{
public:
    // This Macro is required for all classes derived from DFModel
    DECLARE_MODEL_INTERFACE(RADAR_VelocityMeas);

    // Constructor to initialize parameters
    RADAR_VelocityMeas();

    //-------- Function Overloads --------
    virtual bool Setup();
    virtual bool Run();

    // Ports
    SystemVueModelBuilder::CircularBuffer<double> input, Velocity;
    SystemVueModelBuilder::CircularBuffer<int> Index;

    // Parameter
    double PRI;
    int CPI_Num;
    double SampleRate;
    double fc;

    int PRINum;
    int portRate;
};
