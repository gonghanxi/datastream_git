#pragma once
#include "ModelBuilder.h"
#include "TimedDFModel.h"
#include "TimedCircularBuffer.h"

class ConstFxp : public SystemVueModelBuilder::TimedDFModel
{
public:
	enum SelectedShowAdvancedParams { No, Yes };
	enum SelectedSampleRateOption { UnTimed, TimedFromSampleRate, TimedFromSchematic };

	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( ConstFxp );

	// Constructor to initialize parameters
	ConstFxp();
	
	//-------- Function Overloads --------
	virtual bool	Setup();
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::TimedCircularBuffer< double > output;
	
	// Parameter
	double Value;
	SelectedShowAdvancedParams	ShowAdvancedParams;
	SelectedSampleRateOption	SampleRateOption;
	double SampleRate;
	int InitialDelay;

	// С����λ��
	int FxpPos;

};
