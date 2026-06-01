#pragma once
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API FFT_Shift : public SystemVueModelBuilder::DFModel
{
public:
	enum SelectedDirection { FFTShift, IFFTShift };

	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( FFT_Shift );

	// Constructor to initialize parameters
	FFT_Shift();
	
	//-------- Function Overloads --------
	virtual bool	Setup();

	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer< std::complex<double> > input, output;

	// Parameter
	int	FFTSize;
	SelectedDirection	Direction;
};
