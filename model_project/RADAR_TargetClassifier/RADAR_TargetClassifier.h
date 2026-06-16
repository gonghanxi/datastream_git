#pragma once
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API RADAR_TargetClassifier : public SystemVueModelBuilder::DFModel
{
public:
    enum SelectedClassifierType { Kmeans };
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( RADAR_TargetClassifier );

	// Constructor to initialize parameters
	RADAR_TargetClassifier();
	
	//-------- Function Overloads --------
	virtual bool	Setup();
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer< std::complex<double> > trainIn;
	SystemVueModelBuilder::CircularBuffer< std::complex<double> > predictIn;
	SystemVueModelBuilder::CircularBuffer< std::complex<double> > centroid;
	SystemVueModelBuilder::CircularBuffer< int > predictOut;
	
	// Parameter
	SelectedClassifierType ClassifierType;
	int K;
	int TrainSize;
	int PredictSize;
	int MaxIteration;

};
