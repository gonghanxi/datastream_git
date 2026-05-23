#pragma once

#include "ModelBuilder.h"
#include "DFModel.h"
#include "CircularBuffer.h"

namespace SystemVueModelBuilder
{
    class SYSTEMVUEMODELBUILDER_API CrossCorr : public DFModel
	{
	public:
		enum CorrelationTypeEnum
		{
			NonCircular = 0,
			Circular = 1
		};

		enum NormalizationEnum
		{
			None = 0,
			UnBiased = 1,
			Biased = 2
		};

		CircularBuffer<double> input;  
		CircularBuffer<double> input2;  
		CircularBuffer<double> output;  
		CircularBuffer<int>    delay;   

		CorrelationTypeEnum CorrelationType;   
		int                 CorrelationLength; 
		int                 StartLag;          
		int                 StopLag;           
		NormalizationEnum   Normalization;     

		CrossCorr();

		virtual bool Setup() override;
		virtual bool Initialize() override;
		virtual bool Run() override;
		virtual bool Finalize() override;

		virtual bool UpdateDynamicParameters();

		DECLARE_MODEL_INTERFACE(CrossCorr);
	};
}
