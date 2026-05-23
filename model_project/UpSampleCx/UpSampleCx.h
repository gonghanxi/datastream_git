#pragma once
#include "ModelBuilder.h"
#include "DFModel.h"
#include "CircularBuffer.h"
#include <cstddef>
#include <cmath>
#include <complex>
#include "SystemVue.h"

class SYSTEMVUEMODELBUILDER_API UpSampleCx : public SystemVueModelBuilder::DFModel {
public:
	enum ModeEnum { Insertzeros, Holdsample };

	DECLARE_MODEL_INTERFACE(UpSampleCx);

	UpSampleCx();

	virtual bool Setup();
	virtual bool Initialize();
	virtual bool Run();

	SystemVueModelBuilder::CircularBuffer<std::complex<double>> input;
	SystemVueModelBuilder::CircularBuffer<std::complex<double>> output;

	int Factor;
	ModeEnum Mode;
	int Phase;

private:
	bool m_bIsInRun;
};
