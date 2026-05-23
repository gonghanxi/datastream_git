#pragma once
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API Reciprocal : public SystemVueModelBuilder::DFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( Reciprocal );

	// Constructor to initialize parameters
	Reciprocal();
	
	//-------- Function Overloads --------
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer< double > input, output;
	
	// Parameter
	double MagLimit;

};
