#pragma once
#include "ModelBuilder.h"
#include "EnvelopeSignal.h"
#include "TimedDFModel.h"

class SYSTEMVUEMODELBUILDER_API AddEnv_M : public SystemVueModelBuilder::TimedDFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( AddEnv_M );

	// Constructor to initialize parameters
	AddEnv_M();
	
	//-------- Function Overloads --------
	ERESULT PropagateCharacterizationFrequency();
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::EnvelopeMatrixCircularBufferBus	input;
	SystemVueModelBuilder::EnvelopeMatrixCircularBuffer		output;

	// Parameters
	int ChannelNum;
};
