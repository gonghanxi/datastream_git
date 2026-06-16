#pragma once
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API RADAR_TargetTrack_M : public SystemVueModelBuilder::DFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( RADAR_TargetTrack_M );

	// Constructor to initialize parameters
	RADAR_TargetTrack_M();
	
	//-------- Function Overloads --------
	virtual bool	Setup();
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer< SystemVueModelBuilder::Matrix<std::complex<double>> > input;
	SystemVueModelBuilder::CircularBuffer< SystemVueModelBuilder::Matrix<bool> > isTrack;
	SystemVueModelBuilder::CircularBuffer< SystemVueModelBuilder::Matrix<double> > GateStart;
	SystemVueModelBuilder::CircularBuffer< SystemVueModelBuilder::Matrix<double> > Range;
	SystemVueModelBuilder::CircularBuffer< SystemVueModelBuilder::Matrix<std::complex<double>> > output;

	// Parameter
	double PRI_Or_WaveGate;
	double TrackGate;
	double InitGateStartTime;
	double SampleRate;

private:
	int PRINum;
	double GateStartTime;
	int numRows;
	int numCols;
};
