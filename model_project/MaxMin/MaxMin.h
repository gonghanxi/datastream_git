#pragma once
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API MaxMin : public SystemVueModelBuilder::DFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( MaxMin );
    enum SelectedMaxOrMin { min, max };
    enum SelectedCompare { valueIn, magnitudeIn };
    enum SelectedOutputType { valueOut, magnitudeOut };

	// Constructor to initialize parameters
	MaxMin();
	
	//-------- Function Overloads --------
	virtual bool	Setup();
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer< double > input, output;
	SystemVueModelBuilder::CircularBuffer< int > index;
	
	// Parameter
	int N;
	SelectedMaxOrMin MaxOrMin;
	SelectedCompare Compare;
	SelectedOutputType OutputType;
};
