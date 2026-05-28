#pragma once
#include "ModelBuilder.h"
#include "EnvelopeSignal.h"
#include "TimedDFModel.h"

class SYSTEMVUEMODELBUILDER_API SwitchSPST : public SystemVueModelBuilder::TimedDFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( SwitchSPST );

	// Constructor to initialize parameters
	SwitchSPST();
	
	//-------- Function Overloads --------
	virtual bool	Setup();
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::EnvelopeCircularBuffer input, control, output;
	
	// Parameter
	double Loss;
	double Isolation;
	double VThreshold;
	double TOn;
	double TOff;

private:
	bool SwitchState;
	double Ts;
};
