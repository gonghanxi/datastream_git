#pragma once
#include "ModelBuilder.h"
#include "EnvelopeSignal.h"
#include "TimedDFModel.h"

class SYSTEMVUEMODELBUILDER_API PeakDetector : public SystemVueModelBuilder::TimedDFModel
{
public:
	enum SelectedPolarity { positive, negative };


	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( PeakDetector );

	// Constructor to initialize parameters
	PeakDetector();
	
	//-------- Function Overloads --------
	virtual bool	Setup();
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::EnvelopeCircularBuffer	input, output;
	
	// Parameter
	double ChargeTimeConstant;
	double DecayTimeConstant;
	double VThreshold;
	double VTransWidth;
	SelectedPolarity Polarity;

	double polaritySign;
	double VSignal;
	double VDetect;
	double VOut;
	double VTest;
	double SampleRate;
};
