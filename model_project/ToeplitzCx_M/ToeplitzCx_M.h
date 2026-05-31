#pragma once
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API ToeplitzCx_M : public SystemVueModelBuilder::DFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( ToeplitzCx_M );

	// Constructor to initialize parameters
	ToeplitzCx_M();
	
	//-------- Function Overloads --------
	virtual bool	Setup();
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer< std::complex<double> > input;
	SystemVueModelBuilder::CircularBuffer< SystemVueModelBuilder::Matrix<std::complex<double>> > output;

	// Parameters
	int NumRows;
	int NumCols;
};
