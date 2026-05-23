#pragma once
#include "ModelBuilder.h"
#include "TimedDFModel.h"
#include "TimedCircularBuffer.h"
#include "SystemVue.h"

class SYSTEMVUEMODELBUILDER_API ConstInt : public SystemVueModelBuilder::TimedDFModel
{
public:
    enum SelectedShowAdvancedParams { No, Yes };
	enum SelectedSampleRateOption { UnTimed, TimedFromSampleRate, TimedFromSchematic };

public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( ConstInt );

	// Constructor to initialize parameters
	ConstInt();
	
	//-------- Function Overloads --------
	virtual bool	Setup();
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::TimedCircularBuffer< int >	output;
	
	// Parameter
	int Value;
	SelectedShowAdvancedParams	ShowAdvancedParams;
	SelectedSampleRateOption	SampleRateOption;
	double SampleRate;
	int InitialDelay;
};
