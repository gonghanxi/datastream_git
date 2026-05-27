#pragma once
#include "ModelBuilder.h"
#include "EnvelopeSignal.h"
#include "TimedDFModel.h"

class SYSTEMVUEMODELBUILDER_API SwitchSPDT : public SystemVueModelBuilder::TimedDFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( SwitchSPDT );

	// Constructor to initialize parameters
	SwitchSPDT();
	
	//-------- Function Overloads --------
	virtual bool	Setup();
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::EnvelopeCircularBuffer input, control, output1, output2;
	
	// Parameter
	double Loss1;
	double Isolation1;
	double Loss2;
	double Isolation2;
	double VThreshold;
	double TOn1;
	double TOff1;
	double TOn2;
	double TOff2;

private:
	bool SwitchState;
	double Ts;
};
