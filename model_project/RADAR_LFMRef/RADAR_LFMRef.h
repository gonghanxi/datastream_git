#pragma once
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API RADAR_LFMRef : public SystemVueModelBuilder::DFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( RADAR_LFMRef );

	// Constructor to initialize parameters
	RADAR_LFMRef();
	
	//-------- Function Overloads --------
	virtual bool	Setup();
	virtual bool	Run();

	// FFT Function
	void fft(SystemVueModelBuilder::Matrix<std::complex<double>>& a, int n, int invert);

	// Ports
	SystemVueModelBuilder::CircularBuffer< std::complex<double> > output;
	
	// Parameter
	double Pulsewidth;
	double Bandwidth;
	double FM_Offset;
	double SampleRate;
	int FFTSize;

};
