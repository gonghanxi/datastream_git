#pragma once
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API ModuloInt : public SystemVueModelBuilder::DFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( ModuloInt );

	// Constructor to initialize parameters
	ModuloInt();
	
	//-------- Function Overloads --------
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer< int > input, output;
	
	// Parameter
	int moduloValue;

};
