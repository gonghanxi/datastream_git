#pragma once
#include "ModelBuilder.h"
#include "TimedDFModel.h"
#include "TimedCircularBuffer.h"

class SYSTEMVUEMODELBUILDER_API Hysteresis : public SystemVueModelBuilder::TimedDFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( Hysteresis );

	// Constructor to initialize parameters
	Hysteresis();
	
	//-------- Function Overloads --------
	virtual bool	Setup();
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::TimedCircularBuffer< double > input, output;
	
	// Parameter
	double Bandwidth;
	double Backlash;
	double Gain;

	double SampleRate;
	double InternalState;
	double Difference;
};
