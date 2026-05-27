#pragma once
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API RADAR_EWJamming : public SystemVueModelBuilder::DFModel
{
	//enum SelectedJammingType{ BarrageJamming, SpotJamming, MultiSpotJamming, SweptSpotJamming };

public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( RADAR_EWJamming );

	// Constructor to initialize parameters
	RADAR_EWJamming();
	
	//-------- Function Overloads --------
	virtual bool	Setup();
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer< std::complex<double> > jamming;
	
	// Parameter
	//SelectedJammingType JammingType;
	int SampleNum;
	double SampleRate;
	double Mean;
	double Stdev;
	//double Bandwidth;
	//double SweepFreqStep;
	//SystemVueModelBuilder::Matrix<double> MultiSpotBand;
	//int FilterTapsLength;
	double System_Loss;
	//double Atmospheric_Loss_Factor;
};
