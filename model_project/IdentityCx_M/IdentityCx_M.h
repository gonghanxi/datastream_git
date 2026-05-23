#pragma once
#include "ModelBuilder.h"
#include "EnvelopeSignal.h"
#include "TimedDFModel.h"

class SYSTEMVUEMODELBUILDER_API IdentityCx_M : public SystemVueModelBuilder::TimedDFModel
{
public:
	enum SelectedShowAdvancedParams { No, Yes };
	enum SelectedSampleRateOption { UnTimed, TimedFromSampleRate, TimedFromSchematic };

public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( IdentityCx_M );

	// Constructor to initialize parameters
	IdentityCx_M();
	
	//-------- Function Overloads --------
	virtual bool	Setup();
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::TimedCircularBuffer< SystemVueModelBuilder::Matrix<std::complex<double>> > output;
	
	// Parameter
	int RowsCols;
	SelectedShowAdvancedParams ShowAdvancedParams;
	SelectedSampleRateOption SampleRateOption;
	double SampleRate;
	int InitialDelay;

};
