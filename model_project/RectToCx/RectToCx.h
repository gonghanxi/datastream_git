#pragma once
#include "SystemVue.h"
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API RectToCx : public SystemVueModelBuilder::DFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( RectToCx );

	// Constructor to initialize parameters
	RectToCx();
	
	//-------- Function Overloads --------
	virtual bool	Run();

	// Ports
    SystemVueModelBuilder::CircularBuffer<double>	Real, Imag;
	SystemVueModelBuilder::CircularBuffer<std::complex<double>>	Cx;

};
