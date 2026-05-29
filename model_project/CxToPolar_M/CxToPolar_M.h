#pragma once
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API CxToPolar_M : public SystemVueModelBuilder::DFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( CxToPolar_M );

	// Constructor to initialize parameters
	CxToPolar_M();
	
	//-------- Function Overloads --------
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer<SystemVueModelBuilder::Matrix<std::complex<double>>>	input;
	SystemVueModelBuilder::CircularBuffer<SystemVueModelBuilder::Matrix<double>>	magnitude, phase;

};
