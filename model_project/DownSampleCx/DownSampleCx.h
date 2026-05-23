#pragma once
#include "ModelBuilder.h"
#include "SystemVue.h"
#include <complex>

class SYSTEMVUEMODELBUILDER_API DownSampleCx : public SystemVueModelBuilder::DFModel
{
public:
	DECLARE_MODEL_INTERFACE(DownSampleCx);

	DownSampleCx();

	virtual bool Setup();
	virtual bool Run();

	SystemVueModelBuilder::CircularBuffer<std::complex<double>> input;
	SystemVueModelBuilder::CircularBuffer<std::complex<double>> output;

	int Factor;
	int Phase;
};
