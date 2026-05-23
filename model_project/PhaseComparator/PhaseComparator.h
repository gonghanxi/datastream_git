#pragma once

#include "ModelBuilder.h"
#include "EnvelopeSignal.h"
#include "TimedDFModel.h"

#include <complex>

class SYSTEMVUEMODELBUILDER_API PhaseComparator : public SystemVueModelBuilder::TimedDFModel
{
public:
	enum PhaseCharacteristicTypeEnum
	{
		PhaseFreq = 0,   
		Sinusoidal = 1,   
		Triangular = 2    
	};

public:
	DECLARE_MODEL_INTERFACE(PhaseComparator);

	PhaseComparator();

	ERESULT PropagateCharacterizationFrequency();
	virtual bool Initialize() override;
	virtual bool Run() override;

	SystemVueModelBuilder::EnvelopeCircularBuffer s1;     
	SystemVueModelBuilder::EnvelopeCircularBuffer s2;     
	SystemVueModelBuilder::EnvelopeCircularBuffer output; 

	PhaseCharacteristicTypeEnum PhaseCharacteristicType;  
	double GainConstant;                                  
	double MaxAngle;                                      

//private:
	double fcOut_;   

	static double WrapDegreeSymmetric(double angleDeg, double maxAbsDeg); 
	static double WrapToPi(double xRad);                                  
	static double TriangularPhase(double thetaRad);                       
};
