#pragma once
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API CyclicShiftCx : public SystemVueModelBuilder::DFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( CyclicShiftCx );

	// Constructor to initialize parameters
	CyclicShiftCx();
	
	//-------- Function Overloads --------
	virtual bool	Setup();
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer< std::complex<double> > input, output;

	// Parameter
	int BlockSize;
	int Offset;

};
