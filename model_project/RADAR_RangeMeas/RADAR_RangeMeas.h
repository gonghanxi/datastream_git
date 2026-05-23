#pragma once
#include "SystemVue.h"
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API RADAR_RangeMeas : public SystemVueModelBuilder::DFModel
{
public:
    // This Macro is required for all classes derived from DFModel
    DECLARE_MODEL_INTERFACE(RADAR_RangeMeas);

    // Constructor to initialize parameters
    RADAR_RangeMeas();

    //-------- Function Overloads --------
    virtual bool Setup();
    virtual bool Run();

    // Ports
    SystemVueModelBuilder::CircularBuffer<double> input, Range;
    SystemVueModelBuilder::CircularBuffer<int> Index;

    // Parameter
    double PRI;
    int CPI_Num;
    double SampleRate;

    int PRINum;
    int portRate;
};
