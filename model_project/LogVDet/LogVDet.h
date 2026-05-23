#pragma once
#include "ModelBuilder.h"
#include "EnvelopeSignal.h"
#include "TimedDFModel.h"

class SYSTEMVUEMODELBUILDER_API LogVDet : public SystemVueModelBuilder::TimedDFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( LogVDet );

	// Constructor to initialize parameters
	LogVDet();
	
	//-------- Function Overloads --------
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::EnvelopeCircularBuffer	input, output;

	// Parameter
	double Sensitivity;
	double PMin;
	double E;
	double Ec;
	double RefR;

};
