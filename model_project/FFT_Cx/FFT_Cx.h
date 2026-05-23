#pragma once
#include "ModelBuilder.h"
#include "SystemVue.h"

class SYSTEMVUEMODELBUILDER_API FFT_Cx : public SystemVueModelBuilder::DFModel
{
public:
	enum SelectedDirection { FFT, IFFT };
	enum SelectedFreqSequence { O_pos_neg, neg_O_pos };

	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE(FFT_Cx);

	// Constructor to initialize parameters
	FFT_Cx();

	//-------- Function Overloads --------
	virtual bool Setup();
	virtual bool Run();

	void fft(SystemVueModelBuilder::Matrix<std::complex<double>>& a, int n, int invert);

	// Ports
	SystemVueModelBuilder::CircularBuffer< std::complex<double> > input, output;

	// Parameter
	int FFTSize;
	int Size;
	SelectedDirection Direction;
	SelectedFreqSequence FreqSequence;

	int DirectionSign;
};
