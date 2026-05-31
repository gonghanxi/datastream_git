#pragma once
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API Latch : public SystemVueModelBuilder::DFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( Latch );

	// Constructor to initialize parameters
	Latch();
	
	//-------- Function Overloads --------
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer< double > input, output;
	SystemVueModelBuilder::CircularBuffer< int > clock;

	// Parameters
	double storedValue;
};
