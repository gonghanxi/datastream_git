#pragma once

#include "SystemVue.h"
#include "ModelBuilder.h"
#include "TimedDFModel.h"
#include "TimedCircularBuffer.h"
#include "EnvelopeSignal.h"
#include "Matrix.h"

#include <complex>
#include <vector>
#include <algorithm>

class SYSTEMVUEMODELBUILDER_API RADAR_MultiCH_Tx : public SystemVueModelBuilder::TimedDFModel
{
public:
	using Cx = std::complex<double>;
	using EnvSig = SystemVueModelBuilder::EnvelopeSignal;

	using CxBuf = SystemVueModelBuilder::TimedCircularBuffer<Cx>;
	using CxBus = SystemVueModelBuilder::CircularBufferBusT<CxBuf>;

	using EnvBus = SystemVueModelBuilder::EnvelopeCircularBufferBus;

	DECLARE_MODEL_INTERFACE(RADAR_MultiCH_Tx);
	RADAR_MultiCH_Tx();

	bool Setup() override;
	bool Run() override;

	// 用于对齐输出时间轴
	ERESULT CalculateLatency() override;

	// Envelope用于传播Fc
	ERESULT PropagateCharacterizationFrequency() override;

	CxBus  input;   
	EnvBus output;  

	int NumOfCH; 

	// Complex array每通道不平衡系数
	SystemVueModelBuilder::Matrix<Cx> ImbalanceCoef;

	double TStep;    
	double FCarrier; 

private:
	int nChExpected_ = 0;      
	size_t inBusSize_ = 0;      
	size_t outBusSize_ = 0;      
	std::vector<Cx> imbCache_;      // 缓存的每路不平衡系数

	Cx getImbalance_(int k) const;

	// 设置输出元数据：StartTime/TimeStep/Fc
	void applyOutputTiming_(double startTime);
	void applyOutputFc_();
};
