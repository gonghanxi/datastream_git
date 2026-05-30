#pragma once
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API MxCom_M : public SystemVueModelBuilder::DFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( MxCom_M );

	// Constructor to initialize parameters
	MxCom_M();
	
	//-------- Function Overloads --------
	virtual bool	Setup();
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer< SystemVueModelBuilder::Matrix<double> > input, output;
	
	// Parameter
	int OutputNumRows;
	int OutputNumCols;
	int InputNumRows;
	int InputNumCols;

};
