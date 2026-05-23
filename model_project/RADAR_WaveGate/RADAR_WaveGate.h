#pragma once
#include "ModelBuilder.h"

class RADAR_WaveGate : public SystemVueModelBuilder::DFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( RADAR_WaveGate );

	// Constructor to initialize parameters
	RADAR_WaveGate();
	
	//-------- Function Overloads --------
	virtual bool	Setup();
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer< std::complex<double> > input;
	SystemVueModelBuilder::CircularBuffer< double > GateStartCtrl;
	SystemVueModelBuilder::CircularBuffer< std::complex<double> > output;

	// Parameter
	double PRF;
	double StartTime;
	double GateTime;
	double SampleRate;
};
