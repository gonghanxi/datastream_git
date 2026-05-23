#pragma once
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API TransposeCx : public SystemVueModelBuilder::DFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( TransposeCx );

	// Constructor to initialize parameters
	TransposeCx();
	
	//-------- Function Overloads --------
	virtual bool	Setup();
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer< std::complex<double> > input, output;

	// Parameter
	int SamplesInRow;
	int NumberOfRows;
};
