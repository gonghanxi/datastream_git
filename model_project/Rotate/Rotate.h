#pragma once
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API Rotate : public SystemVueModelBuilder::DFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( Rotate );

	// Constructor to initialize parameters
	Rotate();
	
	//-------- Function Overloads --------
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer< std::complex<double> > input, output;
	
	// Parameter
	double RotationAngle;

};
