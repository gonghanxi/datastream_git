#pragma once
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API SubMxCx_M : public SystemVueModelBuilder::DFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( SubMxCx_M );

	// Constructor to initialize parameters
	SubMxCx_M();
	
	//-------- Function Overloads --------
	virtual bool	Setup();
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer< SystemVueModelBuilder::Matrix<std::complex<double>> > input, output;

	// Parameter
	int StartRow;
	int StartCol;
	int NumRows;
	int NumCols;
};
