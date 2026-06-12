#pragma once
#include "ModelBuilder.h"
#include "EnvelopeSignal.h"
#include "TimedDFModel.h"

class SYSTEMVUEMODELBUILDER_API RADAR_RaisedCosineFilter : public SystemVueModelBuilder::TimedDFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( RADAR_RaisedCosineFilter );

	// Constructor to initialize parameters
	RADAR_RaisedCosineFilter();
	
	//-------- Function Overloads --------
	virtual bool	Setup();
	virtual bool	Run();

	SystemVueModelBuilder::Matrix<double> convolve(SystemVueModelBuilder::Matrix<double>& A, SystemVueModelBuilder::Matrix<double>& B, int LenA, int LenB);
	SystemVueModelBuilder::Matrix<double> raisedCosine(double alpha, int numTaps);

	// Ports
	SystemVueModelBuilder::EnvelopeCircularBuffer input, output;
	SystemVueModelBuilder::CircularBuffer<std::complex<double>> coeff;

	// Parameter
	double Alpha;
	double PRI;
	int FilterLen;
	double SampleRate;

private:
	int numPRI;
};
