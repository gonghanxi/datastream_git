#pragma once
#include "SystemVue.h"
#include "ModelBuilder.h"
#include "TimedDFModel.h"
#include "TimedCircularBuffer.h"
#include "EnvelopeSignal.h"
#include <cstddef> 
#include <cmath>


class SYSTEMVUEMODELBUILDER_API UpSampleEnv : public SystemVueModelBuilder::TimedDFModel {
public:
	enum ModeEnum { Insertzeros, Holdsample, Polyphasefilter, Linear };

	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE(UpSampleEnv);

	// Constructor to initialize parameters
	UpSampleEnv();

	//-------- Function Overloads --------
	ERESULT PropagateCharacterizationFrequency();

	virtual bool Setup();
	virtual bool Initialize();
	virtual bool Run();

	std::vector<double> DesignPolyphaseFilter(int factor, float excessBW, int numTaps);
	float kaiserWindow(float n, float beta, int numTaps);
	float besselI0(float x);

	// Ports
	SystemVueModelBuilder::EnvelopeCircularBuffer input;
	SystemVueModelBuilder::EnvelopeCircularBuffer output;

	// Parameters
	int Factor;
	ModeEnum Mode;
	int Phase;
	double ExcessBW;

private:
	bool m_bIsInRun;
	double FcOut;

	// 缓存前一个输入的envelope样本（用于线性插值）
	std::complex<double> currentEnv;
	std::complex<double> m_prevEnv;
	SystemVueModelBuilder::EnvelopeCircularBuffer interpEnv;

	// 标记是否有缓存的前样本（第一次输入时为false）
	bool m_hasPrevSample = false;
};



