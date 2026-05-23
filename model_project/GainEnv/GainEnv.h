#pragma once
#include "ModelBuilder.h"
#include "EnvelopeSignal.h"
#include "SystemVue.h"

class SYSTEMVUEMODELBUILDER_API GainEnv : public SystemVueModelBuilder::DFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( GainEnv );

	// Constructor to initialize parameters
	GainEnv();
	
	//-------- Function Overloads --------
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::EnvelopeCircularBuffer input, output;
	
	// Parameter
	double m_Gain;

};
