#pragma once
#include "ModelBuilder.h"
#include "TimedDFModel.h"
#include "TimedCircularBuffer.h"
#include <fstream>
#include <iostream>

class SYSTEMVUEMODELBUILDER_API Sink_M : public SystemVueModelBuilder::TimedDFModel
{
public:
	enum SelectedStartStopOption { Auto, Samples, Time };


	// Constructor to initialize parameters
	Sink_M();
	~Sink_M();

	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( Sink_M );
	
	//-------- Function Overloads --------
	virtual bool	Setup();
	virtual bool	Run();
	virtual bool	Finalize();

	// Ports
	SystemVueModelBuilder::TimedCircularBuffer< SystemVueModelBuilder::Matrix<double> > input;

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
	SystemVueModelBuilder::Matrix<double>* m_pdBuffer;
	SystemVueModelBuilder::SinkControl m_control;
	std::ofstream outputFile;
	int numCols;
	int numRows;
};
