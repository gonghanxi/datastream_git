#pragma once

#include "ModelBuilder.h"
#include "Matrix.h"
#include "CircularBuffer.h"

#include <vector>
#include <algorithm>
#include <cstddef>

class Quantizer_M : public SystemVueModelBuilder::DFModel
{
public:
	DECLARE_MODEL_INTERFACE(Quantizer_M);

	Quantizer_M();

	bool Setup() override;
	bool Run()   override;

private:
	bool ValidateParameters();
	void BuildInternalTables();

	int  QuantizeIndex(double x) const;

public:
	SystemVueModelBuilder::CircularBuffer<SystemVueModelBuilder::DoubleMatrix> input;

	SystemVueModelBuilder::CircularBuffer<SystemVueModelBuilder::DoubleMatrix> output;

	SystemVueModelBuilder::CircularBuffer<SystemVueModelBuilder::IntMatrix> stepNumber;

	double* Thresholds;
	int     ThresholdsSize;

	double* Levels;
	int     LevelsSize;

private:
	std::vector<double> m_thresholds; 
	std::vector<double> m_levels;    
};
