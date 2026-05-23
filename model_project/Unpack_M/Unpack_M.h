#pragma once
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API Unpack_M : public SystemVueModelBuilder::DFModel
{
public:
	enum SelectedFormat { ColumnMajor, RowMajor };

public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( Unpack_M );

	// Constructor to initialize parameters
	Unpack_M();
	
	//-------- Function Overloads --------
	virtual bool	Setup();
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer< SystemVueModelBuilder::Matrix<double> > input;
	SystemVueModelBuilder::CircularBuffer< double > output;
	
	// Parameter
	int NumRows;
	int NumCols;
	SelectedFormat Format;

};
