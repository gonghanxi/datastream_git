#pragma once
#include "ModelBuilder.h"
#include <fstream>
#include <iostream>

class SYSTEMVUEMODELBUILDER_API RADAR_Pf_Measurement : public SystemVueModelBuilder::DFModel
{
	//enum SelectedControlSimulation { NO, YES };

public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( RADAR_Pf_Measurement );

	// Constructor to initialize parameters
	RADAR_Pf_Measurement();
	
	//-------- Function Overloads --------
	virtual bool	Setup();
	virtual bool	Run();
	virtual bool	Finalize();

	// Ports
	SystemVueModelBuilder::CircularBuffer< double > input;
	
	// Parameter
	int Start;
	int Stop;

	char* FileName;

	//SelectedControlSimulation ControlSimulation;
private:
	SystemVueModelBuilder::SinkControl m_control;
	int FalseCount;
	std::ofstream outputFile;
};
