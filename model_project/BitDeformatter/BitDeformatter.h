#pragma once
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API BitDeformatter : public SystemVueModelBuilder::DFModel
{
public:
	enum SelectedFormat { NRZ, RZ };

public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( BitDeformatter );

	// Constructor to initialize parameters
	BitDeformatter();
	
	//-------- Function Overloads --------
	virtual bool	Setup();
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer< double > input;
	SystemVueModelBuilder::CircularBuffer< bool > output;

	// Parameter
	int SamplesPerBit;
	SelectedFormat Format;
	double LogicZeroLevel;
	double LogicOneLevel;

};
