#pragma once
#include "ModelBuilder.h"
#include "EnvelopeSignal.h"
#include "TimedDFModel.h"
#include "SystemVue.h"

class SYSTEMVUEMODELBUILDER_API AddEnv : public SystemVueModelBuilder::TimedDFModel
{
public:
    enum SelectedOutputFc { min, max, center, userDefined };

public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( AddEnv );

	// Constructor to initialize parameters
	AddEnv();
	
	//-------- Function Overloads --------
	ERESULT PropagateCharacterizationFrequency();
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::EnvelopeCircularBufferBus	input;
    SystemVueModelBuilder::EnvelopeCircularBuffer		output;
	
	// Parameter
    SelectedOutputFc	OutputFc;
	double	UserDefinedFc;

	double	fcOut;
	double	fc, fcmax, fcmin, fcmean;
};
