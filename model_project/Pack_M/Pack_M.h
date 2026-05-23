#pragma once
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API Pack_M : public SystemVueModelBuilder::DFModel
{
public:
	enum SelectedFormat { ColumnMajor, RowMajor };

public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( Pack_M );

	// Constructor to initialize parameters
	Pack_M();
	
	//-------- Function Overloads --------
	virtual bool	Setup();
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer< double > input;
	SystemVueModelBuilder::CircularBuffer< SystemVueModelBuilder::Matrix<double> > output;

	// Parameter
	int NumRows;
	int NumCols;
	SelectedFormat Format;
};
