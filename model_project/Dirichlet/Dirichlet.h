#pragma once
#include "ModelBuilder.h"
#include "TimedDFModel.h"
#include "TimedCircularBuffer.h"
#include <cmath>

class SYSTEMVUEMODELBUILDER_API Dirichlet : public SystemVueModelBuilder::DFModel
{
public:
	DECLARE_MODEL_INTERFACE(Dirichlet);

	Dirichlet();

	bool Setup() override;
	bool Run()   override;

	SystemVueModelBuilder::TimedCircularBuffer<double> input;
	SystemVueModelBuilder::TimedCircularBuffer<double> output;

	int N;

	int DomainFlag;

	int NormalizeFlag;

	int InputMapping;
};
