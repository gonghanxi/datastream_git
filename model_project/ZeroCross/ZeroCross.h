#pragma once
#include "ModelBuilder.h"

class ZeroCross : public SystemVueModelBuilder::DFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( ZeroCross );

	// Constructor to initialize parameters
	ZeroCross();
	
	//-------- Function Overloads --------
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer< double > input, output;
	
	// Parameter
private:
	double previousInput;
	bool isCross;

};
