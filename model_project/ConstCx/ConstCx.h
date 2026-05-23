#pragma once
#include "ModelBuilder.h"
#include "TimedDFModel.h"
#include "TimedCircularBuffer.h"
#include "SystemVue.h"

class SYSTEMVUEMODELBUILDER_API ConstCx : public SystemVueModelBuilder::TimedDFModel
{
public:
    enum SelectedShowAdvancedParams { No, Yes };
	enum SelectedSampleRateOption { UnTimed, TimedFromSampleRate, TimedFromSchematic };

public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( ConstCx );

	// Constructor to initialize parameters
	ConstCx();
	
	//-------- Function Overloads --------
	virtual bool	Setup();
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::TimedCircularBuffer< std::complex<double> >	output;
	
	// Parameter
	std::complex<double> Value;
	SelectedShowAdvancedParams	ShowAdvancedParams;
	SelectedSampleRateOption	SampleRateOption;
	double SampleRate;
	int InitialDelay;
};
