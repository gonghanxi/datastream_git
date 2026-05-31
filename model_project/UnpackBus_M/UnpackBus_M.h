#pragma once
#include "ModelBuilder.h"

class UnpackBus_M : public SystemVueModelBuilder::DFModel
{
public:
	enum SelectedFormat { ColumnMajor, RowMajor };


	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( UnpackBus_M );

	// Constructor to initialize parameters
	UnpackBus_M();
	
	//-------- Function Overloads --------
	virtual bool	Setup();
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer< SystemVueModelBuilder::Matrix<double> > input;
	SystemVueModelBuilder::DoubleCircularBufferBus output;
	
	// Parameter
	int NumRows;
	int NumCols;
	SelectedFormat Format;
};
