#pragma once

#include "ModelBuilder.h"
#include "TimedDFModel.h"
#include "EnvelopeSignal.h"

#include <complex>
#include <vector>

class SYSTEMVUEMODELBUILDER_API EnvFcChange_M : public SystemVueModelBuilder::TimedDFModel
{
public:
	static constexpr double kPI = 3.14159265358979323846;

	DECLARE_MODEL_INTERFACE(EnvFcChange_M);
	EnvFcChange_M();

	bool    Setup() override;
	bool    Run()   override;
	ERESULT PropagateCharacterizationFrequency();

	SystemVueModelBuilder::EnvelopeMatrixCircularBuffer input;
	SystemVueModelBuilder::EnvelopeMatrixCircularBuffer output;

	double OutputFc;
	double Bandwidth;

private:
	double fc_in_;
	double fc_out_;

	// input fc = 0 时，内置帮助文档说明需要 I/Q 提取 + 低通滤波。
	// 这里保存每个矩阵元素对应的低通滤波状态。
	std::vector<std::complex<double> > lpfState_;
	bool   lpfInitialized_;
	size_t lpfNumElements_;

private:
	void resetLpfStateIfNeeded(size_t numElements);
	double getEffectiveBandwidth() const;
	double getInputTimeStep() const;
};
