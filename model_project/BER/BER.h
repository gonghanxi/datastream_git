#pragma once
#include "ModelBuilder.h"
#include "TimedDFModel.h"
#include "TimedCircularBuffer.h"
#include <fstream>
#include <iostream>

class BER : public SystemVueModelBuilder::TimedDFModel
{
public:
	enum SelectedStartStopOption { Auto, Samples, Time };


	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( BER );

	// Constructor to initialize parameters
	BER();
	
	//-------- Function Overloads --------
	virtual bool	Setup();
	virtual bool	Run();
	virtual bool	Finalize();

	// Ports
	SystemVueModelBuilder::CircularBuffer< int > ref, test;
	
	// Parameter
	SelectedStartStopOption StartStopOption;
	int SampleStart;
	int SampleStop;
	double TimeStart;
	double TimeStop;
	int StatusUpdatePeriod;

	char* FileName;
	double SampleRate;

//private:
	int SinkIndex;
	int ResultIndex;
	int PeriodIndex;
	int BitErrorCount;
	SystemVueModelBuilder::SinkControl m_control;
	std::ofstream outputFile;
};
