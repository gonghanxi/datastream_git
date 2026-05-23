#pragma once
#include "ModelBuilder.h"
#include "EnvelopeSignal.h"
#include "TimedDFModel.h"
#include "SystemVue.h"
#include <complex>

class SYSTEMVUEMODELBUILDER_API MpyEnv : public SystemVueModelBuilder::TimedDFModel
{
public:
	enum SelectedFcOut
	{
		min = 0,
		max,
		center,
		userDefined
	};

	DECLARE_MODEL_INTERFACE(MpyEnv);

	MpyEnv();

	ERESULT PropagateCharacterizationFrequency();
	virtual bool Run();
	virtual bool Setup();

	SystemVueModelBuilder::EnvelopeCircularBufferBus input;
	SystemVueModelBuilder::EnvelopeCircularBuffer      output;

	SelectedFcOut FcOut;        
	double        UserDefinedFc;  

	double fc;       
	double fcmax;     
	double fcmin;     
	double fcmean;    
	double fcOut;     
};
