#pragma once
#include "ModelBuilder.h"
#include "EnvelopeSignal.h"
#include "TimedDFModel.h"
#include <fstream>
#include <iostream>

class SYSTEMVUEMODELBUILDER_API SinkEnv_M : public SystemVueModelBuilder::TimedDFModel
{
public:
	enum SelectedStartStopOption { Auto, Samples, Time };


	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( SinkEnv_M );

	// Constructor to initialize parameters
	SinkEnv_M();
	~SinkEnv_M();
	
	//-------- Function Overloads --------
	ERESULT PropagateCharacterizationFrequency();
	virtual bool	Setup();
	virtual bool	Run();
	virtual bool	Finalize();

	// Ports
	SystemVueModelBuilder::EnvelopeMatrixCircularBuffer input;

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
	SystemVueModelBuilder::EnvelopeMatrix* m_pdBuffer;
	SystemVueModelBuilder::SinkControl m_control;
	std::ofstream outputFile;
	double Fc;
	int numCols;
	int numRows;
};
