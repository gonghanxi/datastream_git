#pragma once
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API Toeplitz_M : public SystemVueModelBuilder::DFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( Toeplitz_M );

	// Constructor to initialize parameters
	Toeplitz_M();
	
	//-------- Function Overloads --------
	virtual bool	Setup();
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer< double > input;
	SystemVueModelBuilder::CircularBuffer< SystemVueModelBuilder::Matrix<double> > output;

	// Parameters
	int NumRows;
	int NumCols;
};
