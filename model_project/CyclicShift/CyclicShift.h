#pragma once
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API CyclicShift : public SystemVueModelBuilder::DFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( CyclicShift );

	// Constructor to initialize parameters
	CyclicShift();
	
	//-------- Function Overloads --------
	virtual bool	Setup();
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer< double > input, output;
	
	// Parameter
	int BlockSize;
	int Offset;

};
