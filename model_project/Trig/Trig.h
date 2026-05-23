#pragma once
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API Trig : public SystemVueModelBuilder::DFModel
{
public:
	enum SelectedFunctionType { Sin, Cos, Tan, Cot, Asin, Acos, Atan, Acot, Sinh, Cosh, Tanh, Coth, Asinh, Acosh, Atanh, Acoth };

public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( Trig );

	// Constructor to initialize parameters
	Trig();
	
	//-------- Function Overloads --------
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer< double > input, output;
	
	// Parameter
	SelectedFunctionType FunctionType;
};
