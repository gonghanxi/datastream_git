#pragma once
#include "ModelBuilder.h"
#include "EnvelopeSignal.h"
#include "TimedDFModel.h"

class SYSTEMVUEMODELBUILDER_API RADAR_Clutter_H : public SystemVueModelBuilder::TimedDFModel
{
	enum SelectedPDF { Rayleigh, Lognoraml, Weibull };
	enum SelectedPSD { Gaussian, Cauchy, Allpole };

public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( RADAR_Clutter_H );

	// Constructor to initialize parameters
	RADAR_Clutter_H();
	
	//-------- Function Overloads --------
	ERESULT PropagateCharacterizationFrequency();
	virtual bool	Setup();
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::EnvelopeCircularBuffer	input;

	SystemVueModelBuilder::CircularBuffer<std::complex<double>>	Coeff;
	SystemVueModelBuilder::EnvelopeCircularBuffer	output;
	SystemVueModelBuilder::EnvelopeCircularBuffer	ClutterSample;

	// Parameter
	double RF_Freq;
	SelectedPDF PDF;
	double VA;
	double VB;
	SelectedPSD PSD;
	double PA;
	double PB;
	double TStep;
	int FilterLen;
	double DurationTime;
	double Vr;

private:
	int num_sample;
};
