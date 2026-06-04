#pragma once
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API PcwzLinear : public SystemVueModelBuilder::DFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( PcwzLinear );

	// Constructor to initialize parameters
	PcwzLinear();
	
	//-------- Function Overloads --------
	virtual bool	Setup();
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer< double > input, output;
	
	// Parameter
	SystemVueModelBuilder::Matrix<std::complex<double> >	Breakpoints;

	int numBreakpoints;
	std::complex<double>	currentBreakpoint;
	SystemVueModelBuilder::Matrix<double> 	slope;
	SystemVueModelBuilder::Matrix<double> 	intercept;
	double x1, y1, x2, y2;
};
