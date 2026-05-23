#pragma once
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API PolynomialInt : public SystemVueModelBuilder::DFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( PolynomialInt );

	// Constructor to initialize parameters
	PolynomialInt();
	
	//-------- Function Overloads --------
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer< int > input, output;
	
	// Parameter
	SystemVueModelBuilder::Matrix<int>	Coefficients;

};
