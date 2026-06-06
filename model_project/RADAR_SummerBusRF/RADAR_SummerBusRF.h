#pragma once
#include "ModelBuilder.h"
#include "EnvelopeSignal.h"
#include "TimedDFModel.h"

class SYSTEMVUEMODELBUILDER_API RADAR_SummerBusRF : public SystemVueModelBuilder::TimedDFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( RADAR_SummerBusRF );

    enum SelectedFcOut { min, center, max };

	// Constructor to initialize parameters
	RADAR_SummerBusRF();
	
	//-------- Function Overloads --------
	ERESULT PropagateCharacterizationFrequency();
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::EnvelopeCircularBufferBus	input1, input2, output;
	
	// Parameter
	SelectedFcOut	FcOut;

	double	fc1, fc2, fcOut;
};
