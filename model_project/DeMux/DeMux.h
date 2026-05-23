#pragma once
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API DeMux : public SystemVueModelBuilder::DFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( DeMux );

	// Constructor to initialize parameters
	DeMux();
	
	//-------- Function Overloads --------
	virtual bool	Setup();
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer< double > input;
	SystemVueModelBuilder::CircularBuffer< int > control;
	SystemVueModelBuilder::CircularBufferBusT< SystemVueModelBuilder::CircularBuffer<double> > output;

	// Parameter
	double BlockSize;

};
