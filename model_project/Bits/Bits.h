#pragma once
#include "ModelBuilder.h"
#include "TimedDFModel.h"
#include "TimedCircularBuffer.h"
#include "SystemVue.h"

class SYSTEMVUEMODELBUILDER_API Bits : public SystemVueModelBuilder::TimedDFModel
{
public:
	// 是否显示高级参数
	enum ShowAdvancedParamsEnum { NO, YES };
	enum SampleRateOptionEnum { UnTimed, TimedfromSampleRate, TimedfromSchematic };
	enum BurstModeEnum { OFF, Single, Multiple };
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( Bits );

	// Constructor to initialize parameters
	Bits();
	
	//-------- Function Overloads --------
	virtual bool Setup();
	virtual bool Run();

	// Ports
//	SystemVueModelBuilder::TimedCircularBuffer< bool > output;
    SystemVueModelBuilder::TimedCircularBuffer< int > output;
	
	// Parameter
	double ProbOfZero;  // 生成数为0的概率
	double BitRate;		// 输出比特率
	ShowAdvancedParamsEnum ShowAdvancedParams; // 是否显示高级参数（NO：不显示，YES：显示）
	SampleRateOptionEnum SampleRateOption; // 采样率选项
	double SampleRate;  // 采样率
	int InitialDelay;   // 初始延迟
	BurstModeEnum BurstMode;  // 突发模式
	int BurstLength;    // 突发长度
	int BurstPeriod;    // 突发周期
	int BurstDelay;     // 突发延迟

	// 临时变量
	bool previousBitValue; // 记录上一个Bit的值
};
