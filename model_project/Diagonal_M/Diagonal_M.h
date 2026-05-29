#pragma once
#include "ModelBuilder.h"
#include "TimedDFModel.h"
#include "TimedCircularBuffer.h"
#include "Matrix.h"

class SYSTEMVUEMODELBUILDER_API Diagonal_M : public SystemVueModelBuilder::DFModel
{
public:
	enum ShowAdvancedEnum {
		ShowAdv_No = 0,
		ShowAdv_Yes = 1
	};

	enum SampleRateOptionEnum {
		SRO_UnTimed = 0,
		SRO_TimedFromSampleRate = 1,
		SRO_TimedFromSchematic = 2
	};

	DECLARE_MODEL_INTERFACE(Diagonal_M);

	Diagonal_M();

	virtual bool Setup() override;
	virtual bool Run()   override;

	SystemVueModelBuilder::TimedCircularBuffer<
		SystemVueModelBuilder::Matrix<double>
	> output;

	SystemVueModelBuilder::Matrix<double> DiagonalElements;

	ShowAdvancedEnum     ShowAdvancedParams;     
	SampleRateOptionEnum SampleRateOption;       

	double               SampleRate;

	int                  InitialDelay;

private:
	SystemVueModelBuilder::Matrix<double> diagMat_;
	SystemVueModelBuilder::Matrix<double> zeroMat_;
	int   N_;         
	int   produced_;  
};
