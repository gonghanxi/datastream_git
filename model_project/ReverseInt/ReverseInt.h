#pragma once
#include "ModelBuilder.h"
#include "SystemVue.h"

class SYSTEMVUEMODELBUILDER_API ReverseInt : public SystemVueModelBuilder::DFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( ReverseInt );

	// Constructor to initialize parameters
	ReverseInt();
	
	//-------- Function Overloads --------
	virtual bool	Setup();

	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer< int > input, output;

	// Parameter
	int N;

};
