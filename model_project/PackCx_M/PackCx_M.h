#pragma once
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API PackCx_M : public SystemVueModelBuilder::DFModel
{
public:
	enum SelectedFormat { ColumnMajor, RowMajor };


	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( PackCx_M );

	// Constructor to initialize parameters
	PackCx_M();
	
	//-------- Function Overloads --------
	virtual bool	Setup();
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer< std::complex<double> > input;
	SystemVueModelBuilder::CircularBuffer< SystemVueModelBuilder::Matrix<std::complex<double>> > output;
	
	// Parameter
	int NumRows;
	int NumCols;
	SelectedFormat Format;

};
