#pragma once

#include "ModelBuilder.h"
#include "DFModel.h"
#include "CircularBuffer.h"
#include "Matrix.h"
#include <vector>

namespace SystemVueModelBuilder
{
	using DoubleStream = CircularBuffer<double>;
	using DoubleStreamBus = CircularBufferBusT<DoubleStream>;

    class SYSTEMVUEMODELBUILDER_API AsyncDistributor : public DFModel
	{
	public:
		AsyncDistributor();

		DoubleStream    input;   
		DoubleStreamBus output; 

		Matrix<int> BlockSizes;

		bool Setup() override;
		bool Run()   override;

		DECLARE_MODEL_INTERFACE(AsyncDistributor);

	private:
		std::vector<int> m_blockSizes;
	};
}
