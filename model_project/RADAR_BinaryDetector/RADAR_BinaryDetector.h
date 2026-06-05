#pragma once
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API RADAR_BinaryDetector : public SystemVueModelBuilder::DFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( RADAR_BinaryDetector );

	// Constructor to initialize parameters
	RADAR_BinaryDetector();
	
	//-------- Function Overloads --------
	virtual bool	Setup();
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer< double > input;
	SystemVueModelBuilder::CircularBuffer< int > output;

	// Parameter
	double Threshold;
	double PRI;
	double SampleRate;

private:
	int numPRI;
};
