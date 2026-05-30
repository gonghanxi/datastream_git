#pragma once
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API Limit_M : public SystemVueModelBuilder::DFModel
{
public:
	// This Macro is required for all classes derived from DFModel
    enum SelectedLimiterType { linear, atan };
	DECLARE_MODEL_INTERFACE( Limit_M );

	// Constructor to initialize parameters
	Limit_M();
	
	//-------- Function Overloads --------
	virtual bool	Setup();
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer< SystemVueModelBuilder::Matrix<double> >	input, output;
	
	// Parameter
	double K;
	double Bottom;
	double Top;
	SelectedLimiterType LimiterType;
};
