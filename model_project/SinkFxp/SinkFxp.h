#pragma once
#include "SystemVue.h"
#include "ModelBuilder.h"
#include "TimedDFModel.h"
#include "TimedCircularBuffer.h"
#include <fstream>
#include <iomanip>
#include <iostream>

class SYSTEMVUEMODELBUILDER_API SinkFxp : public SystemVueModelBuilder::TimedDFModel
{
public:
	enum SelectedStartStopOption { Auto, Samples, Time };

	// Constructor to initialize parameters
	SinkFxp();
	~SinkFxp();

	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( SinkFxp );
	
	//-------- Function Overloads --------
	virtual bool	Setup();
	virtual bool	Run();
	virtual bool	Finalize();

	// Ports
	SystemVueModelBuilder::TimedCircularBuffer< double > input;

	// Parameter
	int FxpPos;

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
