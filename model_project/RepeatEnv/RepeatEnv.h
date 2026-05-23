#pragma once
#include "ModelBuilder.h"
#include "EnvelopeSignal.h"
#include "TimedDFModel.h"

class SYSTEMVUEMODELBUILDER_API RepeatEnv : public SystemVueModelBuilder::TimedDFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( RepeatEnv );

	// Constructor to initialize parameters
	RepeatEnv();
	
	//-------- Function Overloads --------
	virtual bool	Setup();
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::EnvelopeCircularBuffer input, output;
	
	// Parameter
	double NumTimes;
	double BlockSize;

};
