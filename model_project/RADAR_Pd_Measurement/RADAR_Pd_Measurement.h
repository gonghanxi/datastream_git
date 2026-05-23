#pragma once
#include "ModelBuilder.h"
#include <fstream>
#include <iostream>

class SYSTEMVUEMODELBUILDER_API RADAR_Pd_Measurement : public SystemVueModelBuilder::DFModel
{
	//enum SelectedControlSimulation { NO, YES };

public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( RADAR_Pd_Measurement );

	// Constructor to initialize parameters
	RADAR_Pd_Measurement();
	
	//-------- Function Overloads --------
	virtual bool	Setup();
	virtual bool	Run();
	virtual bool	Finalize();

	// Ports
	SystemVueModelBuilder::CircularBuffer< double > input;
	
	// Parameter
	int Start;
	int PRI_NUM;
	int FFT_Size;
	int DetectionNum;
	int TargetsInPRI;
	double TargetThreshold;

	char* FileName;
	//SelectedControlSimulation ControlSimulation;

private:
	bool DetectStatus;
	int DetectCount;
	SystemVueModelBuilder::SinkControl m_control;
	std::ofstream outputFile;
};
