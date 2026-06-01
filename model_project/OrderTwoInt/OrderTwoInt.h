#pragma once
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API OrderTwoInt : public SystemVueModelBuilder::DFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( OrderTwoInt );

	// Constructor to initialize parameters
	OrderTwoInt();
	
	//-------- Function Overloads --------
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer< bool > upper, lower, greater, lesser;

};
