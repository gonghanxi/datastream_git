#pragma once
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API SlidWinAvg : public SystemVueModelBuilder::DFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( SlidWinAvg );

	// Constructor to initialize parameters
	SlidWinAvg();
	
	//-------- Function Overloads --------
	virtual bool	Setup();
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer< double > input, output;
	
	// Parameter
	int WindowSize;

	SystemVueModelBuilder::Matrix<double> slideWindow;
	int currentIndex;
	double currentSum;
};
