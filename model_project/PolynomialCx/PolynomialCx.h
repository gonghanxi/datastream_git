#pragma once
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API PolynomialCx : public SystemVueModelBuilder::DFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( PolynomialCx );

	// Constructor to initialize parameters
	PolynomialCx();
	
	//-------- Function Overloads --------
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer< std::complex<double> > input, output;
	
	// Parameter
	SystemVueModelBuilder::Matrix< std::complex<double> >	Coefficients;

};
