#pragma once
#include "ModelBuilder.h"
#include "EnvelopeSignal.h"

class SYSTEMVUEMODELBUILDER_API TransposeEnv : public SystemVueModelBuilder::DFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( TransposeEnv );

	// Constructor to initialize parameters
	TransposeEnv();
	
	//-------- Function Overloads --------
	virtual bool	Setup();
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::EnvelopeCircularBuffer input, output;
	
	// Parameter
	int SamplesInRow;
	int NumberOfRows;
};
