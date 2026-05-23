#pragma once
#include "ModelBuilder.h"
#include "EnvelopeSignal.h"
#include "SystemVue.h"

class SYSTEMVUEMODELBUILDER_API ReverseEnv : public SystemVueModelBuilder::DFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( ReverseEnv );

	// Constructor to initialize parameters
	ReverseEnv();
	
	//-------- Function Overloads --------
	virtual bool	Setup();
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::EnvelopeCircularBuffer input, output;
	
	// Parameter
	int N;

};
