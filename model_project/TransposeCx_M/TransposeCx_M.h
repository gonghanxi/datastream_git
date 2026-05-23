#pragma once
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API TransposeCx_M : public SystemVueModelBuilder::DFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( TransposeCx_M );

	// Constructor to initialize parameters
	TransposeCx_M();
	
	//-------- Function Overloads --------
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer< SystemVueModelBuilder::Matrix<std::complex<double>> > input, output;

};
