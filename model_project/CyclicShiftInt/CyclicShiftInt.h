#pragma once
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API CyclicShiftInt : public SystemVueModelBuilder::DFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( CyclicShiftInt );

	// Constructor to initialize parameters
	CyclicShiftInt();
	
	//-------- Function Overloads --------
	virtual bool	Setup();
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer< int > input, output;

	// Parameter
	int BlockSize;
	int Offset;

};
