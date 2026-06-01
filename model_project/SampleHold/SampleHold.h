#pragma once
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API SampleHold : public SystemVueModelBuilder::DFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( SampleHold );

	// Constructor to initialize parameters
	SampleHold();
	
	//-------- Function Overloads --------
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer< double > input, output;
	SystemVueModelBuilder::CircularBuffer< int > clock;

};
