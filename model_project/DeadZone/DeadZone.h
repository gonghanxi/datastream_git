#pragma once
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API DeadZone : public SystemVueModelBuilder::DFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( DeadZone );

	// Constructor to initialize parameters
	DeadZone();
	
	//-------- Function Overloads --------
	virtual bool	Setup();
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer< double > input, output;
	
	// Parameter
	double K;
	double Low;
	double High;
};
