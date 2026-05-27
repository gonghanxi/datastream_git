#pragma once
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API RADAR_EWDeceptionJamming : public SystemVueModelBuilder::DFModel
{
public:
	DECLARE_MODEL_INTERFACE( RADAR_EWDeceptionJamming );

	// Constructor to initialize parameters
	RADAR_EWDeceptionJamming();
	
	//-------- Function Overloads --------
	virtual bool	Setup();
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer< std::complex<double> > signal;
	SystemVueModelBuilder::CircularBuffer< std::complex<double> > jamming;

	// Parameter
	int SampleNum;
	double SampleRate;
	int FalseTargetNum;
	double MaxRange;
	double System_Loss;
	SystemVueModelBuilder::Matrix<double> FalseTargetRangeDelay;
	SystemVueModelBuilder::Matrix<double> FalseTargetDopplerOffset;
	SystemVueModelBuilder::Matrix<double> FalseTargetGain;

private:
	SystemVueModelBuilder::Matrix<std::complex<double>> FalseTargetDelayBuffer;
	int MaxSampleNum;
	int SampleIndex;
};
