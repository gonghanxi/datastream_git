#pragma once
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API Mux : public SystemVueModelBuilder::DFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( Mux );

	// Constructor to initialize parameters
	Mux();
	
	//-------- Function Overloads --------
	virtual bool	Setup();
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::DoubleCircularBufferBus input;
	SystemVueModelBuilder::CircularBuffer< int > control;
	SystemVueModelBuilder::CircularBuffer< double > output;

	// Parameter
	int BlockSize;

};
