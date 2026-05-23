#pragma once
#include "SystemVue.h"
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API CxToPolar : public SystemVueModelBuilder::DFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( CxToPolar );

	// Constructor to initialize parameters
	CxToPolar();
	
	//-------- Function Overloads --------
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer<std::complex<double>>	input;
	SystemVueModelBuilder::CircularBuffer<double>	magnitude, phase;

};
