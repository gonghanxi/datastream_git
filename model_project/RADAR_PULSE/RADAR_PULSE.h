#pragma once
#include "ModelBuilder.h"
#include "SystemVue.h"
#include "TimedDFModel.h"
//#include "DFModel.h"
#include "TimedCircularBuffer.h"
//#include "CircularBuffer.h"
#include <complex>

class SYSTEMVUEMODELBUILDER_API RADAR_PULSE : public SystemVueModelBuilder::TimedDFModel//TimedDFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE(RADAR_PULSE);

	// Constructor to initialize parameters
	RADAR_PULSE();

	//-------- Function Overloads --------
	virtual bool	Setup();

	virtual bool	Run();

	// 模型端口定义
	SystemVueModelBuilder::TimedCircularBuffer< double > output;

	// 模型参数定义
	SystemVueModelBuilder::Matrix<double> Pulsewidth;
	SystemVueModelBuilder::Matrix<double> PRI;
	SystemVueModelBuilder::Matrix<int> PRI_Combination;
	double SampleRate;

	// 信号计数器
	int counter;

	// 测试变量
	//double testVAR;
};
