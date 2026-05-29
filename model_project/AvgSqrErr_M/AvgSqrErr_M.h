#pragma once
#include "ModelBuilder.h"
#include "DFModel.h"
#include "CircularBuffer.h"
#include "Matrix.h"

#include <cstddef>   
#include <cmath>     
#include <vector>    

class SYSTEMVUEMODELBUILDER_API AvgSqrErr_M : public SystemVueModelBuilder::DFModel {
public:
	DECLARE_MODEL_INTERFACE(AvgSqrErr_M);

	AvgSqrErr_M();
	bool Setup() override;
	bool Run()   override;

	SystemVueModelBuilder::CircularBuffer<
		SystemVueModelBuilder::Matrix<double>
	> input1;

	SystemVueModelBuilder::CircularBuffer<
		SystemVueModelBuilder::Matrix<double>
	> input2;

	SystemVueModelBuilder::CircularBuffer<double> output;

	int NumInputsToAverage;   

private:
	std::vector<double> m_ring;  
	int    m_head;               
	double m_accumSSE;           
	int    m_count;              

	int    m_rows;
	int    m_cols;
	bool   m_shapeInit;
};
