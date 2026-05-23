#pragma once
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API Unwrap : public SystemVueModelBuilder::DFModel
{
public:
	enum SelectedPhaseType { radians, degrees };

public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( Unwrap );

	// Constructor to initialize parameters
	Unwrap();
	
	//-------- Function Overloads --------
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer< double > input, output;
	
	// Parameter
	SelectedPhaseType PhaseType;
	double OutPhase;
	double PrevPhase;
};
