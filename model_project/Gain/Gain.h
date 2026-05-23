#pragma once
#include "SystemVue.h"
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API Gain : public SystemVueModelBuilder::DFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE(Gain);

	// Constructor to initialize parameters
	Gain();

	//-------- Function Overloads --------
	virtual bool Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer< double > input, output;

	// Parameter
	double m_Gain;
};
