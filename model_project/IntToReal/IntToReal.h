#pragma once
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API IntToReal : public SystemVueModelBuilder::DFModel
{

public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( IntToReal );

	// Constructor to initialize parameters
	IntToReal();
	
	//-------- Function Overloads --------
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer< int > input;
	SystemVueModelBuilder::CircularBuffer< double > output;
	
	// Parameter

};
