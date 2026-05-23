#pragma once
#include "ModelBuilder.h"
#include "TimedDFModel.h"
#include "SystemVue.h"
#include "TimedCircularBuffer.h"
#include <complex>

class SYSTEMVUEMODELBUILDER_API RADAR_LFM : public SystemVueModelBuilder::TimedDFModel//TimedDFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE(RADAR_LFM);

	// Constructor to initialize parameters
	RADAR_LFM();

	//-------- Function Overloads --------
	virtual bool	Setup();

	virtual bool	Run();

	// 模型端口定义
	SystemVueModelBuilder::TimedCircularBuffer< std::complex<double> > output;

	// 模型参数定义
	SystemVueModelBuilder::Matrix<double> Pulsewidth;
	SystemVueModelBuilder::Matrix<double> PRI;
	SystemVueModelBuilder::Matrix<int> PRI_Combination;
	SystemVueModelBuilder::Matrix<double> Bandwidth;
	SystemVueModelBuilder::Matrix<double> FM_Offset;
	double SampleRate;

	// 信号计数器
	int counter;

	// 测试变量
	//double testVAR;
};
