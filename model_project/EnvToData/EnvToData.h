#pragma once
#include "ModelBuilder.h"
#include "EnvelopeSignal.h"
#include "TimedDFModel.h"
#include "SystemVue.h"

class SYSTEMVUEMODELBUILDER_API EnvToData : public SystemVueModelBuilder::TimedDFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE(EnvToData);

	ERESULT PropagateCharacterizationFrequency();
	EnvToData();

	//-------- Function Overloads --------
	virtual bool Run();

	// Ports
	SystemVueModelBuilder::EnvelopeCircularBuffer input;
	SystemVueModelBuilder::CircularBuffer<double> fc, time, I, Q;
};
