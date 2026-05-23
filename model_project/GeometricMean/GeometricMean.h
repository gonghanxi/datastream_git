#pragma once
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API GeometricMean : public SystemVueModelBuilder::DFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( GeometricMean );

	// Constructor to initialize parameters
	GeometricMean();
	
	//-------- Function Overloads --------
	virtual bool	Setup();
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer< double > input, output;
	
	// Parameter
	int N;
	double Gain;

};
