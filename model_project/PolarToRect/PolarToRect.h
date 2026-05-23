#pragma once
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API PolarToRect : public SystemVueModelBuilder::DFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( PolarToRect );

	// Constructor to initialize parameters
	PolarToRect();
	
	//-------- Function Overloads --------
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer<double>	magnitude, phase, x, y;

};
