#pragma once

#include "ModelBuilder.h"
#include "DFModel.h"
#include "CircularBuffer.h"
#include "Matrix.h"

#include <vector>

namespace SystemVueModelBuilder
{
	using IntStream = CircularBuffer<int>;
	using IntStreamBus = CircularBufferBusT<IntStream>;

    class SYSTEMVUEMODELBUILDER_API AsyncDistributorInt : public DFModel
	{
	public:
		AsyncDistributorInt();

		IntStream    input;   
		IntStreamBus output;  

		Matrix<int> BlockSizes;

		bool Setup() override;
		bool Run()   override;

		DECLARE_MODEL_INTERFACE(AsyncDistributorInt);

	private:
		std::vector<int> m_blockSizes;
	};
}
