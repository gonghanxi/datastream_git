#pragma once
#include "ModelBuilder.h"
#include "EnvelopeSignal.h"
#include "TimedDFModel.h"

class SYSTEMVUEMODELBUILDER_API RADAR_ArrayCouple : public SystemVueModelBuilder::TimedDFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( RADAR_ArrayCouple );

	// Constructor to initialize parameters
	RADAR_ArrayCouple();
	
	//-------- Function Overloads --------
	virtual bool	Setup();
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::EnvelopeCircularBufferBus input, output;
	
	// Parameter
	int ChannelNum;
	SystemVueModelBuilder::Matrix < std::complex < double >> CoupleCoef;
};
