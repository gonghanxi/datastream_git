#pragma once
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API TransposeInt : public SystemVueModelBuilder::DFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( TransposeInt );

	// Constructor to initialize parameters
	TransposeInt();
	
	//-------- Function Overloads --------
	virtual bool	Setup();
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer< int > input, output;

	// Parameter
	int SamplesInRow;
	int NumberOfRows;
};
