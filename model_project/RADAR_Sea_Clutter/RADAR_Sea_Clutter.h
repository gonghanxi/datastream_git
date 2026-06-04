#pragma once
#include "ModelBuilder.h"
#include "EnvelopeSignal.h"
#include "TimedDFModel.h"

class SYSTEMVUEMODELBUILDER_API RADAR_Sea_Clutter : public SystemVueModelBuilder::TimedDFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( RADAR_Sea_Clutter );
    enum SelectedSeaState{ SeaState_1, SeaState_2, SeaState_3, SeaState_4, SeaState_5, SeaState_6, SeaState_7 };
    enum SelectedAntenna_Pattern { Gaussian };

	// Constructor to initialize parameters
	RADAR_Sea_Clutter();
	
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
	SelectedSeaState SeaState;
	double RF_Freq;
	SelectedAntenna_Pattern Antenna_Pattern;
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
	double SS;
	double WindVelocity;
	double Sigma;

};
