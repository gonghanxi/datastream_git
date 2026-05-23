#pragma once
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API RepeatCx : public SystemVueModelBuilder::DFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( RepeatCx );

	// Constructor to initialize parameters
	RepeatCx();
	
	//-------- Function Overloads --------
	virtual bool	Setup();
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer< std::complex<double> > input, output;

	// Parameter
	double NumTimes;
	double BlockSize;

};
