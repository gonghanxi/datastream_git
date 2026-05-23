#pragma once
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API Modulo : public SystemVueModelBuilder::DFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( Modulo );

	// Constructor to initialize parameters
	Modulo();
	
	//-------- Function Overloads --------
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer< double > input, output;
	
	// Parameter
	double moduloValue;

};
