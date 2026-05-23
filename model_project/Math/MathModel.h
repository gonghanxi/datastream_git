#pragma once
#include "SystemVue.h"
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API Math : public SystemVueModelBuilder::DFModel
{
public:
	enum SelectedFunctionType { Abs, Ceil, Exp, Floor, Ln, Log10, Pow10, Recip, Round, Sqr, Sqrt, Sgn };
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( Math );

	// Constructor to initialize parameters
	Math();
	
	//-------- Function Overloads --------
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer< double > input, output;
	
	// Parameter
	SelectedFunctionType	FunctionType;

};
