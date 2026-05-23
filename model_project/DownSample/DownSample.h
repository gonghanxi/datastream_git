#pragma once
#include "ModelBuilder.h"
#include "SystemVue.h"

class SYSTEMVUEMODELBUILDER_API DownSample : public SystemVueModelBuilder::DFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE(DownSample);

	// Constructor to initialize parameters
	DownSample();

	//-------- Function Overloads --------
	virtual bool Setup();
	virtual bool Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer< double > input, output;

	// Parameter
	int Factor;
	int Phase;
};
