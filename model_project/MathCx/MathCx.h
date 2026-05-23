#pragma once
#include "SystemVue.h"
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API MathCx : public SystemVueModelBuilder::DFModel
{
public:
	enum SelectedFunctionType{ Abs, Ceil, Exp, Floor, Ln, Log10, Pow10, Recip, Round, Sqr, Sqrt, Conj };
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( MathCx );

	// Constructor to initialize parameters
	MathCx();
	
	//-------- Function Overloads --------
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer< std::complex<double> > input, output;
	
	// Parameter
	SelectedFunctionType	FunctionType;
};
