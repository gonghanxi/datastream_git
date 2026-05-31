#pragma once

#include "ModelBuilder.h"
#include "TimedDFModel.h"
#include "EnvelopeSignal.h"


class SYSTEMVUEMODELBUILDER_API EnvFcChange_M : public SystemVueModelBuilder::TimedDFModel
{
public:
	static constexpr double kPI = 3.14159265358979323846;

	DECLARE_MODEL_INTERFACE(EnvFcChange_M);
	EnvFcChange_M();

	bool    Setup() override;
	bool    Run()   override;
	ERESULT PropagateCharacterizationFrequency();  

	SystemVueModelBuilder::EnvelopeMatrixCircularBuffer input;
	SystemVueModelBuilder::EnvelopeMatrixCircularBuffer output;

	double OutputFc;   
	double Bandwidth;  

private:
	double fc_in_;   
	double fc_out_;  
};
