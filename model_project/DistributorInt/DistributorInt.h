#pragma once

#include "ModelBuilder.h"
#include "DFModel.h"
#include "CircularBuffer.h"

namespace SystemVueModelBuilder
{
	using IntStream = CircularBuffer<int>;
	using IntBus = CircularBufferBusT<IntStream>;

    class SYSTEMVUEMODELBUILDER_API DistributorInt : public DFModel
	{
	public:
		DistributorInt();

		IntStream input;   
		IntBus    output;  

		int BlockSize;

		bool Setup() override;
		bool Run()   override;

		DECLARE_MODEL_INTERFACE(DistributorInt);

	private:
		size_t m_iBlockSize;
	};

} 
