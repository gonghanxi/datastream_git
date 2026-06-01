#pragma once
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API AverageCxWOffset : public SystemVueModelBuilder::DFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( AverageCxWOffset );

	// Constructor to initialize parameters
	AverageCxWOffset();
	
	//-------- Function Overloads --------
	virtual bool	Setup();
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer< std::complex<double> > input, output;
	SystemVueModelBuilder::CircularBuffer< int > Offset;
	
	// Parameter
	int NumInputsToAverage;

	int initialZeros;
	std::complex<double> currentSum;
	std::complex<double> currentAverage;
	int bufferIndex;
};
