#pragma once
#include "ModelBuilder.h"
#include "SystemVue.h"

class SYSTEMVUEMODELBUILDER_API BitFormatter : public SystemVueModelBuilder::DFModel
{
public:
	enum SelectedFormat { NRZ, RZ };

public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( BitFormatter );

	// Constructor to initialize parameters
	BitFormatter();
	
	//-------- Function Overloads --------
	virtual bool	Setup();
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer< bool > input;
	SystemVueModelBuilder::CircularBuffer< double > output;
	
	// Parameter
	int SamplesPerBit;
	SelectedFormat Format;
	double LogicZeroLevel;
	double LogicOneLevel;

};
