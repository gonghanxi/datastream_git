#pragma once
#include "ModelBuilder.h"
#include "EnvelopeSignal.h"
#include "TimedDFModel.h"

class SYSTEMVUEMODELBUILDER_API Splitter_M : public SystemVueModelBuilder::TimedDFModel
{
public:
	enum SelectedMode { SubArray, Custom };

public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( Splitter_M );

	// Constructor to initialize parameters
	Splitter_M();
	
	//-------- Function Overloads --------
	virtual bool	Setup();
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::EnvelopeMatrixCircularBuffer	input, output;
	
	// Parameter
	SelectedMode Mode;
	int NumRows;
	int NumCols;
	SystemVueModelBuilder::Matrix<int> ElementMap;
	double InsertionLoss;

	int inRow;
	int inCol;
	int outRow;
	int outCol;
	int numMap;
	SystemVueModelBuilder::Matrix<int> channelCount;

};
