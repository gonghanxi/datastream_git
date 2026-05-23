#pragma once
#include "ModelBuilder.h"
#include "TimedDFModel.h"
#include "TimedCircularBuffer.h"
#include "SystemVue.h"
//#include <iomanip>
#include <fstream>
#include <iostream>

class SYSTEMVUEMODELBUILDER_API Sink : public SystemVueModelBuilder::TimedDFModel
{
public:
	enum SelectedStartStopOption{ Auto, Samples, Time };

public:
	// Constructor to initialize parameters
	Sink();
	~Sink();

	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( Sink );
	
	//-------- Function Overloads --------
	virtual bool	Setup();
	virtual bool	Run();
	virtual bool	Finalize();

	// Ports
	SystemVueModelBuilder::TimedCircularBuffer< double > input;
	
	// Parameter
	SelectedStartStopOption StartStopOption;
	int SampleStart;
	int SampleStop;
	double TimeStart;
	double TimeStop;

	char* FileName;
	double SampleRate;

private:
	int Index;
	size_t m_iBuffer;
	double* m_pdBuffer;
	SystemVueModelBuilder::SinkControl m_control;
	std::ofstream outputFile;
};


