#pragma once
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API CxToRect_M : public SystemVueModelBuilder::DFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( CxToRect_M );

	// Constructor to initialize parameters
	CxToRect_M();
	
	//-------- Function Overloads --------
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer<SystemVueModelBuilder::Matrix<std::complex<double>>>	input;
	SystemVueModelBuilder::CircularBuffer<SystemVueModelBuilder::Matrix<double>>	real, imag;

};
