#pragma once
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API Transpose : public SystemVueModelBuilder::DFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( Transpose );

	// Constructor to initialize parameters
	Transpose();
	
	//-------- Function Overloads --------
	virtual bool	Setup();
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer< double > input, output;
	
	// Parameter
	int SamplesInRow;
	int NumberOfRows;
};
