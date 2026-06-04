#pragma once
#include "ModelBuilder.h"
#include <fstream>
#include <iostream>

class SYSTEMVUEMODELBUILDER_API RADAR_JammingEffect : public SystemVueModelBuilder::DFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( RADAR_JammingEffect );

    enum SelectedJammingType { CoverJamming, DeceptionJamming };

	// Constructor to initialize parameters
	RADAR_JammingEffect();
	
	//-------- Function Overloads --------
	virtual bool	Setup();
	virtual bool	Run();
	virtual bool	Finalize();

	// Ports
	SystemVueModelBuilder::CircularBuffer< double > input;
	
	// Parameter
	SelectedJammingType JammingType;
	int Start;
	int PRI_NUM;
	int FFT_Size;
	int DetectionNum;
	int TargetsInPRI;
	int FalseTargetNum;
	double TargetThreshold;

	char* FileName;

private:
	bool DetectStatus;
	int DetectCount;
	SystemVueModelBuilder::SinkControl m_control;
	std::ofstream outputFile;
};
