#pragma once
#include "ModelBuilder.h"
#include "EnvelopeSignal.h"
#include "TimedDFModel.h"

class SYSTEMVUEMODELBUILDER_API RADAR_Ground_Clutter : public SystemVueModelBuilder::TimedDFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( RADAR_Ground_Clutter );

    enum SelectedGroundType {Farmland, Desert, Hill, Mountain, UserDefine};
    enum SelectedAntenna_Pattern { Gaussian };

	// Constructor to initialize parameters
	RADAR_Ground_Clutter();
	
	//-------- Function Overloads --------
	ERESULT PropagateCharacterizationFrequency();
	virtual bool	Setup();
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::EnvelopeCircularBuffer	input;
	SystemVueModelBuilder::CircularBuffer<double>	BodyRoll;
	SystemVueModelBuilder::CircularBuffer<double>	BodyPitch;
	SystemVueModelBuilder::CircularBuffer<double>	BodyYaw;
	SystemVueModelBuilder::CircularBuffer<double>	AntTilt;
	SystemVueModelBuilder::CircularBuffer<double>	AntYaw;
	SystemVueModelBuilder::EnvelopeCircularBuffer	output;
	SystemVueModelBuilder::EnvelopeCircularBuffer	ClutterSample;

	// Parameter
	SelectedGroundType GroundType;
	double RF_Freq;
	SelectedAntenna_Pattern Antenna_Pattern;
	double Scatter0;
	double GrazingAngle;
	double BodyRollAngle;
	double BodyPitchAngle;
	double BodyYawAngle;
	double AntTiltAngle;
	double AntYawAngle;
	double PRF;
	double SampleRate;
	double Antenna_Height;
	double Platform_Velocity;

private:
	int num_sample;
	double phi;
	double lambda;
	double miu;
	double A;
	double B;
	double Beta0;
	double Sigmac;
	double Sigma;

};
