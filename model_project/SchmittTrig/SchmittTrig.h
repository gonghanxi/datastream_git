#pragma once
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API SchmittTrig : public SystemVueModelBuilder::DFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( SchmittTrig );

	// Constructor to initialize parameters
	SchmittTrig();
	
	//-------- Function Overloads --------
	virtual bool	Setup();
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer< double > input, output;
	
	// Parameter
	double ILow;
	double IHigh;
	double OLow;
	double OHigh;
	bool TrigStatus;
	
};
