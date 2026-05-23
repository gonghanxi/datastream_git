#pragma once
#include "ModelBuilder.h"
#include "EnvelopeSignal.h"
#include "TimedDFModel.h"
#include "SystemVue.h"

class SYSTEMVUEMODELBUILDER_API SubEnv : public SystemVueModelBuilder::TimedDFModel
{
public:
	enum SelectedFcOut { min, max, center, userDefined };

	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE(SubEnv);

	// Constructor to initialize parameters
	SubEnv();

	//-------- Function Overloads --------
	ERESULT PropagateCharacterizationFrequency();
	virtual bool Run();

	// Ports
	SystemVueModelBuilder::EnvelopeCircularBuffer pos, output;
	SystemVueModelBuilder::EnvelopeCircularBufferBus neg;

	// Parameter
	SelectedFcOut FcOut;
	double UserDefinedFc;

	double fcOut;
	double fcPos, fcNeg;
	double fcmax, fcmin, fcmean;
};
