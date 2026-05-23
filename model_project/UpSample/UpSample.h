#pragma once
#include "ModelBuilder.h"
#include "DFModel.h"
#include "CircularBuffer.h"
#include <cstddef>
#include <cmath>
#include "SystemVue.h"

class SYSTEMVUEMODELBUILDER_API UpSample : public SystemVueModelBuilder::DFModel {
public:
	enum ModeEnum { Insertzeros, Holdsample };

	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE(UpSample);

	// Constructor to initialize parameters
	UpSample();

	//-------- Function Overloads --------
	virtual bool Setup();
	virtual bool Initialize();
	virtual bool Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer<double> input;
	SystemVueModelBuilder::CircularBuffer<double> output;

	// Parameters
	int Factor;
	ModeEnum Mode;
	int Phase;

private:
	bool m_bIsInRun;
};
