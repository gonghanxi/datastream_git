#pragma once
#include "ModelBuilder.h"
#include "TimedDFModel.h"
#include "TimedCircularBuffer.h"
//#include <iomanip>
#include <fstream>
#include <iostream>
#include "SystemVue.h"

class SYSTEMVUEMODELBUILDER_API SinkInt : public SystemVueModelBuilder::TimedDFModel
{
public:
	enum SelectedStartStopOption{ Auto, Samples, Time };

public:
	// Constructor to initialize parameters
    SinkInt();
    ~SinkInt();

	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( Sink );
	
	//-------- Function Overloads --------
	virtual bool	Setup();
	virtual bool	Run();
	virtual bool	Finalize();

	// Ports
    SystemVueModelBuilder::TimedCircularBuffer< int > input;
	
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
    int* m_pdBuffer;
	SystemVueModelBuilder::SinkControl m_control;
	std::ofstream outputFile;
};


