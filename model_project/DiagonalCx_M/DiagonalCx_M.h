#pragma once
#include "ModelBuilder.h"
#include "TimedDFModel.h"
#include "TimedCircularBuffer.h"
#include "Matrix.h"
#include <complex>

class SYSTEMVUEMODELBUILDER_API DiagonalCx_M : public SystemVueModelBuilder::TimedDFModel
{
public:
	enum ShowAdvancedEnum { ShowAdv_No = 0, ShowAdv_Yes = 1 };
	enum SampleRateOptionEnum { SRO_UnTimed = 0, SRO_TimedFromSampleRate = 1, SRO_TimedFromSchematic = 2 };

	DECLARE_MODEL_INTERFACE(DiagonalCx_M);

	DiagonalCx_M();
	bool Setup() override;
	bool Run()   override;

	SystemVueModelBuilder::TimedCircularBuffer<
		SystemVueModelBuilder::Matrix<std::complex<double>>
	> output;

	SystemVueModelBuilder::Matrix<std::complex<double>> DiagonalElements;
	ShowAdvancedEnum     ShowAdvancedParams;
	SampleRateOptionEnum SampleRateOption;
	double               SampleRate;
	int                  InitialDelay;

private:
	SystemVueModelBuilder::Matrix<std::complex<double>> diagMat_;
	SystemVueModelBuilder::Matrix<std::complex<double>> zeroMat_;
	int   N_;
	int   produced_;
};
