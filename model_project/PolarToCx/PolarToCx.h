#pragma once
#include "SystemVue.h"
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API PolarToCx : public SystemVueModelBuilder::DFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( PolarToCx );

	// Constructor to initialize parameters
	PolarToCx();
	
	//-------- Function Overloads --------
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer<double>	magnitude, phase;
	SystemVueModelBuilder::CircularBuffer<std::complex<double>>	output;

};
