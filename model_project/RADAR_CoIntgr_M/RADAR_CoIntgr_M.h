#pragma once

#include "ModelBuilder.h"
#include "Matrix.h"

#include <complex>

class SYSTEMVUEMODELBUILDER_API RADAR_CoIntgr_M : public SystemVueModelBuilder::DFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE(RADAR_CoIntgr_M);

	// Constructor to initialize parameters
	RADAR_CoIntgr_M();

	//-------- Function Overloads --------
	virtual bool Setup();
	virtual bool Run();

	// Ports：矩阵版输入/输出均为 complex matrix，每次 Run 处理 1 个矩阵
	SystemVueModelBuilder::CircularBuffer<
		SystemVueModelBuilder::Matrix<std::complex<double> >
	> input, output;

	// Parameter：矩阵版只保留 NumOfPulse
	int NumOfPulse;
};
