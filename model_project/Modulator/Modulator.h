#pragma once
#include "ModelBuilder.h"
#include "TimedDFModel.h"
#include "TimedCircularBuffer.h"
#include "EnvelopeSignal.h"
#include "SystemVue.h"
#include <complex>
#include <limits>
#include <cmath>

class SYSTEMVUEMODELBUILDER_API Modulator : public SystemVueModelBuilder::TimedDFModel {
public:
	enum InputTypeEnum { InIQ = 0, InAmpPhase = 1, InAmpFreq = 2 };
	enum ConjQuadEnum { CQ_No = 0, CQ_Yes = 1 };
	enum MirrorEnum { Mirror_No = 0, Mirror_Yes = 1 };

	enum ShowIQEnum { ShowIQ_NO = 0, ShowIQ_YES = 1 };

	DECLARE_MODEL_INTERFACE(Modulator);
	Modulator();

	bool    Setup() override;
	bool    Run()   override;
	ERESULT PropagateCharacterizationFrequency() override;

	SystemVueModelBuilder::TimedCircularBuffer<double> input1;       // 可选
	SystemVueModelBuilder::TimedCircularBuffer<double> input2;       // 可选
	SystemVueModelBuilder::EnvelopeCircularBuffer      LO;           // 可选
	SystemVueModelBuilder::EnvelopeCircularBuffer      output;       // 必连
	SystemVueModelBuilder::EnvelopeCircularBuffer      quad_output;  // 必连

	InputTypeEnum  InputType;
	double         FCarrier;          
	double         InitialPhase;      
	double         AmpSensitivity;    
	double         PhaseSensitivity;  
	double         FreqSensitivity;   
	ConjQuadEnum   ConjugatedQuadrature;
	MirrorEnum     MirrorSignal;
	ShowIQEnum     ShowIQ_Impairments;

	double GainImbalance;  
	double PhaseImbalance; 
	double I_OriginOffset; 
	double Q_OriginOffset; 
	double IQ_Rotation;    

private:
	double phaseAcc_;     
	double lastTime_;     

	static inline double deg2rad(double d) { return d * 3.14159265358979323846 / 180.0; }
};
