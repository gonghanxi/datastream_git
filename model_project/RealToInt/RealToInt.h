#pragma once
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API RealToInt : public SystemVueModelBuilder::DFModel
{
public:
    enum SelectedConvertType { Static_Cast, Floor, Ceil, Round };
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( RealToInt );

	// Constructor to initialize parameters
	RealToInt();
	
	//-------- Function Overloads --------
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer< double > input;
	SystemVueModelBuilder::CircularBuffer< int > output;
	
	// Parameter
	SelectedConvertType ConvertType;

};
