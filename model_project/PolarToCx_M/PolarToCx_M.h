#pragma once
#include "ModelBuilder.h"

class PolarToCx_M : public SystemVueModelBuilder::DFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( PolarToCx_M );

	// Constructor to initialize parameters
	PolarToCx_M();
	
	//-------- Function Overloads --------
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer<SystemVueModelBuilder::Matrix<double>>	magnitude, phase;
	SystemVueModelBuilder::CircularBuffer<SystemVueModelBuilder::Matrix<std::complex<double>>>	output;

};
