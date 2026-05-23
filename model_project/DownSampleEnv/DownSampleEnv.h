#pragma once
#include "ModelBuilder.h"
#include "TimedDFModel.h"
#include "EnvelopeSignal.h"
#include "SystemVue.h"

class SYSTEMVUEMODELBUILDER_API DownSampleEnv : public SystemVueModelBuilder::TimedDFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( DownSampleEnv );

	// Constructor to initialize parameters
	DownSampleEnv();
	
	//-------- Function Overloads --------
	ERESULT PropagateCharacterizationFrequency();
	virtual bool	Setup();
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::EnvelopeCircularBuffer input, output;
	
	// Parameter
	int Factor;
	int Phase;

};
