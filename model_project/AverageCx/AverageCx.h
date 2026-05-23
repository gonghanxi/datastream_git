#pragma once
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API AverageCx : public SystemVueModelBuilder::DFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( AverageCx );

	// Constructor to initialize parameters
	AverageCx();
	
	//-------- Function Overloads --------
	virtual bool	Setup();
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer< std::complex<double> > input, output;
	
	// Parameter
	int NumInputsToAverage;
	int BlockSize;
};
