#pragma once
#include "ModelBuilder.h"
#include "EnvelopeSignal.h"
#include "TimedDFModel.h"

class SYSTEMVUEMODELBUILDER_API AddNDensity : public SystemVueModelBuilder::TimedDFModel
{
	//enum SelectedNDensityType{ Constant_noise_density, Noise_density_vs_freq };

public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( AddNDensity );

	// Constructor to initialize parameters
	AddNDensity();
	
	//-------- Function Overloads --------
	virtual bool	Setup();
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::EnvelopeCircularBuffer input, output;
	
	// Parameter
	//SelectedNDensityType NDensityType;
	double NDensity;
	//SystemVueModelBuilder::Matrix<double> NDensityFreq;
	//SystemVueModelBuilder::Matrix<double> NDensityPower;
	double RefR;

};
