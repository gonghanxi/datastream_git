#pragma once
#include "SystemVue.h"
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API CxToRect : public SystemVueModelBuilder::DFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( CxToRect );

	// Constructor to initialize parameters
	CxToRect();
	
	//-------- Function Overloads --------
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer<std::complex<double>>	Cx;
	SystemVueModelBuilder::CircularBuffer<double>	Real, Imag;

};
