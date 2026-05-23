#pragma once
#include "ModelBuilder.h"
#include "EnvelopeSignal.h"
#include "TimedDFModel.h"

class SYSTEMVUEMODELBUILDER_API EnvToCx_M : public SystemVueModelBuilder::TimedDFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( EnvToCx_M );

	// Constructor to initialize parameters
	EnvToCx_M();
	
	//-------- Function Overloads --------
	ERESULT PropagateCharacterizationFrequency();
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::EnvelopeMatrixCircularBuffer input;
	SystemVueModelBuilder::EnvelopeCircularBuffer fc;
	SystemVueModelBuilder::CircularBuffer<SystemVueModelBuilder::Matrix<std::complex<double>>> output;
	

};
