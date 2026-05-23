#pragma once
#include "ModelBuilder.h"
#include "EnvelopeSignal.h"
#include "TimedDFModel.h"
//#include <iomanip>
#include <fstream>
#include <iostream>
#include "SystemVue.h"

class SYSTEMVUEMODELBUILDER_API SinkEnv : public SystemVueModelBuilder::TimedDFModel
{
public:
	enum SelectedStartStopOption { Auto, Samples, Time };

public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( SinkEnv );

	// Constructor to initialize parameters
	SinkEnv();
	~SinkEnv();
	
	//-------- Function Overloads --------
	ERESULT PropagateCharacterizationFrequency();
	virtual bool	Setup();
	virtual bool	Run();
	virtual bool	Finalize();

	// Ports
	SystemVueModelBuilder::EnvelopeCircularBuffer input;
	
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
	std::complex<double>* m_pdBuffer;
	SystemVueModelBuilder::SinkControl m_control;
	std::ofstream outputFile;
};
