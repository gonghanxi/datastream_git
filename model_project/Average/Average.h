#pragma once
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API Average : public SystemVueModelBuilder::DFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( Average );

	// Constructor to initialize parameters
	Average();
	
	//-------- Function Overloads --------
	virtual bool	Setup();
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer< double > input, output;
	
	// Parameter
	int NumInputsToAverage;
	int BlockSize;
};
