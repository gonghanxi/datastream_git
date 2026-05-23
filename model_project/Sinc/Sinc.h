#pragma once
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API Sinc : public SystemVueModelBuilder::DFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( Sinc );

	// Constructor to initialize parameters
	Sinc();
	
	//-------- Function Overloads --------
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer< double > input, output;
	
	// Parameter

};
