#pragma once
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API DownSampleVarPhase : public SystemVueModelBuilder::DFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( DownSampleVarPhase );

	// Constructor to initialize parameters
	DownSampleVarPhase();
	
	//-------- Function Overloads --------
	virtual bool	Setup();
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer< double > input, output;
	SystemVueModelBuilder::CircularBuffer< int > phase;

	// Parameter
	int Factor;

};
