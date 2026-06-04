#pragma once
#include "ModelBuilder.h"
#include "EnvelopeSignal.h"
#include "TimedDFModel.h"

class RADAR_EchoGenerator : public SystemVueModelBuilder::TimedDFModel
{
public:
	enum SelectedIncludePropagationEffect { No, Yes };


	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( RADAR_EchoGenerator );

	// Constructor to initialize parameters
	RADAR_EchoGenerator();
	
	//-------- Function Overloads --------
	virtual bool	Setup();
	virtual bool	Run();

	// Ports
    SystemVueModelBuilder::EnvelopeCircularBufferBus inSignal;
    SystemVueModelBuilder::CircularBufferBusT<SystemVueModelBuilder::CircularBuffer<SystemVueModelBuilder::Matrix<double>>> TxPlatformLoc;
	SystemVueModelBuilder::CircularBufferBusT<SystemVueModelBuilder::CircularBuffer<SystemVueModelBuilder::Matrix<double>>> RxPlatformLoc;
	SystemVueModelBuilder::CircularBufferBusT<SystemVueModelBuilder::CircularBuffer<SystemVueModelBuilder::Matrix<double>>> TargetScatterLoc;
	SystemVueModelBuilder::DoubleCircularBufferBus TargetScatterRCS;
	
	SystemVueModelBuilder::EnvelopeCircularBufferBus TargetSignal;
	SystemVueModelBuilder::EnvelopeCircularBufferBus outSignal;
	SystemVueModelBuilder::EnvelopeCircularBufferBus RxSignal;

	// Parameter
	double SampleRate;
	double SystemLoss;
	SelectedIncludePropagationEffect IncludePropagationEffect;
	double RF_Freq;
	int SimulationSampleNum;

private:
	int	TargetNum;
	int TxPlatformNum;
	int RxPlatformNum;
	int ChannelNum;

	int Index;
	SystemVueModelBuilder::DComplexMatrix TargetDelayBuffer;
	SystemVueModelBuilder::DComplexMatrix outDelayBuffer;
	SystemVueModelBuilder::DComplexMatrix RxDelayBuffer;
};
