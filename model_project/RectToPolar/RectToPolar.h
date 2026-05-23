#pragma once
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API RectToPolar : public SystemVueModelBuilder::DFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( RectToPolar );

	// Constructor to initialize parameters
	RectToPolar();
	
	//-------- Function Overloads --------
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer<double>	x, y, magnitude, phase;

};
