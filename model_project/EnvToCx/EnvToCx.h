#pragma once
#include "SystemVue.h"
#include "ModelBuilder.h"
#include "EnvelopeSignal.h"
#include "TimedDFModel.h"
#include <complex>

class SYSTEMVUEMODELBUILDER_API EnvToCx : public SystemVueModelBuilder::TimedDFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( EnvToCx );

	// Constructor to initialize parameters
	EnvToCx();
	
	//-------- Function Overloads --------
	ERESULT PropagateCharacterizationFrequency();
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::EnvelopeCircularBuffer	Env;
	SystemVueModelBuilder::EnvelopeCircularBuffer	Fc;
	SystemVueModelBuilder::CircularBuffer<std::complex<double>>	Cx;

	// Parameter


};
