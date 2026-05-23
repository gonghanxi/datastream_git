#ifndef CXTOENV_H
#define CXTOENV_H

#pragma once
#include "SystemVue.h"
#include "ModelBuilder.h"

#include "EnvelopeSignal.h"
#include "TimedDFModel.h"

class SYSTEMVUEMODELBUILDER_API CxToEnv : public SystemVueModelBuilder::TimedDFModel
{
public:
    // This Macro is required for all classes derived from DFModel
    DECLARE_MODEL_INTERFACE(CxToEnv);

    // Constructor to initialize parameters
    CxToEnv();

    //-------- Function Overloads --------
    ERESULT PropagateCharacterizationFrequency();
    virtual bool	Run();

    // Ports
    SystemVueModelBuilder::CircularBuffer<std::complex<double>>	Cx;
    SystemVueModelBuilder::EnvelopeCircularBuffer	Env;
    SystemVueModelBuilder::EnvelopeCircularBuffer	Fc;

    // Parameter
    double fc;
};


#endif // CXTOENV_H
