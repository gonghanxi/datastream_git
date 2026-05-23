#pragma once
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API Transpose_M : public SystemVueModelBuilder::DFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( Transpose_M );

	// Constructor to initialize parameters
	Transpose_M();
	
	//-------- Function Overloads --------
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer< SystemVueModelBuilder::Matrix<double> > input, output;

};
