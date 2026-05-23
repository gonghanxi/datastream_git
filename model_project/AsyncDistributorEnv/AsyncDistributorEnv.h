#pragma once

#include "ModelBuilder.h"
#include "EnvelopeSignal.h"
#include "DFModel.h"
#include "Matrix.h"
#include <vector>

namespace SystemVueModelBuilder
{
	using EnvelopeStream = EnvelopeCircularBuffer;      
	using EnvelopeStreamBus = EnvelopeCircularBufferBus; 

    class SYSTEMVUEMODELBUILDER_API AsyncDistributorEnv : public DFModel
	{
	public:
		AsyncDistributorEnv();

		EnvelopeStream    input;   
		EnvelopeStreamBus output;  

		Matrix<int> BlockSizes;

		bool Setup() override;
		bool Run()   override;

		DECLARE_MODEL_INTERFACE(AsyncDistributorEnv);

	private:
		std::vector<int> m_blockSizes;
	};
}
