#pragma once

#include "ModelBuilder.h"
#include "DFModel.h"
#include "CircularBuffer.h"
#include <vector>

class SYSTEMVUEMODELBUILDER_API Quantizer : public SystemVueModelBuilder::DFModel
{
public:
	DECLARE_MODEL_INTERFACE(Quantizer);

	Quantizer();

	virtual bool Setup();
	virtual bool Run();

	SystemVueModelBuilder::CircularBuffer<double> input;      
	SystemVueModelBuilder::CircularBuffer<double> output;     
	SystemVueModelBuilder::CircularBuffer<int>    stepNumber; 

	double*  Thresholds;      
	int ThresholdsSize;  

	double*  Levels;          
	int LevelsSize;      

private:
	std::vector<double> m_thresholds; 
	std::vector<double> m_levels;     
};
