#pragma once
#include "ModelBuilder.h"
#include "TimedDFModel.h"
#include "TimedCircularBuffer.h"

class SYSTEMVUEMODELBUILDER_API PulseGen : public SystemVueModelBuilder::TimedDFModel
{
public:
	enum EdgeSymmetrys
	{
		Symmetric,
		Asymmetric
	};

	enum Polaritys
	{
		normal,
		inverted
	};

public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( PulseGen );

	// Constructor to initialize parameters
	PulseGen();
	
	//-------- Function Overloads --------
	virtual bool	Setup();
	virtual bool	Run();

	// 模型端口定义
	SystemVueModelBuilder::TimedCircularBuffer< double > output;
	
	// 模型参数定义
	double LoLevel;
	double HiLevel;
	double Period;
	double Phase;
	double PulseWidth;
	EdgeSymmetrys EdgeSymmetry;
	double EdgeTime;
	double RisingEdgeTime;
	double FallingEdgeTime;
	Polaritys Polarity;
	double SampleRate;

	// 信号计数器
	int counter;
};
