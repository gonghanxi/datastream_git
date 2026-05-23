#pragma once
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API Polynomial : public SystemVueModelBuilder::DFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( Polynomial );

	// Constructor to initialize parameters
	Polynomial();
	
	//-------- Function Overloads --------
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer< double > input, output;
	
	// Parameter
	SystemVueModelBuilder::Matrix<double>	Coefficients;

};
