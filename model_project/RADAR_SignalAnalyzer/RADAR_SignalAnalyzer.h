#pragma once
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API RADAR_SignalAnalyzer : public SystemVueModelBuilder::DFModel
{
public:
	enum SelectedAnalyzerType{ FFT, IFFT, ACF };
	enum SelectedWindowType{ Rectangle, Bartlett, Hanning, Hamming, Blackman, SteepBlackman, Kaiser };
	enum SelectedCorrType{ Normal, Biased, UnBiased };
	enum SelectedNormalizedType { Normalized, NonNormalized };
	enum SelectedFFTShiftType { Shifted, NonShift };

public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( RADAR_SignalAnalyzer );

	// Constructor to initialize parameters
	RADAR_SignalAnalyzer();
	
	//-------- Function Overloads --------
	virtual bool	Setup();
	virtual bool	Run();

	void fft(SystemVueModelBuilder::Matrix<std::complex<double>>& a, int n, int invert);
	int factorial(int n);
	double I0(int n, double x);
	SystemVueModelBuilder::Matrix<std::complex<double>> autoCorr(SystemVueModelBuilder::Matrix<std::complex<double>>& A, int LenA);

	// Ports
	SystemVueModelBuilder::CircularBuffer< std::complex<double> > input;
	SystemVueModelBuilder::CircularBuffer< double > output;
	
	// Parameter
	SelectedAnalyzerType AnalyzerType;
	SelectedWindowType WindowType;
	double WindowParameter;
	SelectedCorrType CorrType;
	SelectedNormalizedType NormalizedType;
	SelectedFFTShiftType FFTShiftType;
	int SampleNum;
	int FFTSize;
	double SampleRate;
};
