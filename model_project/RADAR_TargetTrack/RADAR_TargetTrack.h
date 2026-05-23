#pragma once
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API RADAR_TargetTrack : public SystemVueModelBuilder::DFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( RADAR_TargetTrack );

	// Constructor to initialize parameters
	RADAR_TargetTrack();
	
	//-------- Function Overloads --------
	virtual bool	Setup();
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer< std::complex<double> > input;
	SystemVueModelBuilder::CircularBuffer< bool > isTrack;
	SystemVueModelBuilder::CircularBuffer< double > GateStart;
	SystemVueModelBuilder::CircularBuffer< double > Range;
	SystemVueModelBuilder::CircularBuffer< std::complex<double> > output;

	// Parameter
	double PRI_Or_WaveGate;
	double TrackGate;
	double InitGateStartTime;
	double SampleRate;

private:
	int PRINum;
	double GateStartTime;
};
