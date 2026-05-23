#pragma once
#include "ModelBuilder.h"
#include "SystemVue.h"

class SYSTEMVUEMODELBUILDER_API Variance : public SystemVueModelBuilder::DFModel
{
public:
	DECLARE_MODEL_INTERFACE(Variance);

	Variance();
	virtual bool Setup();
	virtual bool Run();

	SystemVueModelBuilder::CircularBuffer<double> in, mean, variance;

	int BlockSize;

	double sum;
	double sumSqr;
	int sumN;
};
