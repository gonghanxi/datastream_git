#pragma once
#include "ModelBuilder.h"
#include "SystemVue.h"

class SYSTEMVUEMODELBUILDER_API GainCx : public SystemVueModelBuilder::DFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( GainCx );

	// Constructor to initialize parameters
	GainCx();
	
	//-------- Function Overloads --------
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer< std::complex<double> > input, output;
	
	// Parameter
	double m_Gain;

};
