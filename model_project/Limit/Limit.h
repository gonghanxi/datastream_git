#pragma once
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API Limit : public SystemVueModelBuilder::DFModel
{
public:
	enum SelectedLimiterType{ linear, atan };

public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( Limit );

	// Constructor to initialize parameters
	Limit();
	
	//-------- Function Overloads --------
	virtual bool	Setup();
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer< double > input, output;
	
	// Parameter
	double K;
	double Bottom;
	double Top;
	SelectedLimiterType LimiterType;
};
