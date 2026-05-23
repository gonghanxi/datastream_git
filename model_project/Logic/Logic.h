#pragma once
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API Logic : public SystemVueModelBuilder::DFModel
{
public:
	enum SelectedLogicOperation { NOT, AND, NAND, OR, NOR, XOR, XNOR };

public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( Logic );

	// Constructor to initialize parameters
	Logic();
	
	//-------- Function Overloads --------
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::BoolCircularBufferBus	input;
	SystemVueModelBuilder::CircularBuffer< bool >	output;
	// Parameter
	SelectedLogicOperation	LogicOperation;

};
