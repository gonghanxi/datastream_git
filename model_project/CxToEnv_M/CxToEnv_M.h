#pragma once
#include "ModelBuilder.h"
#include "EnvelopeSignal.h"
#include "TimedDFModel.h"

class SYSTEMVUEMODELBUILDER_API CxToEnv_M : public SystemVueModelBuilder::TimedDFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( CxToEnv_M );

	// Constructor to initialize parameters
	CxToEnv_M();
	
	//-------- Function Overloads --------
	ERESULT PropagateCharacterizationFrequency();
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer< SystemVueModelBuilder::Matrix<std::complex<double>>> input;
	SystemVueModelBuilder::EnvelopeMatrixCircularBuffer output;
	SystemVueModelBuilder::EnvelopeCircularBuffer fc;

	// Parameter
	double Fc;

};
