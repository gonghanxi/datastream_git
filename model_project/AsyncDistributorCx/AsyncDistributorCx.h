#pragma once

#include "ModelBuilder.h"
#include "DFModel.h"
#include "CircularBuffer.h"
#include "Matrix.h"

#include <vector>
#include <complex>

namespace SystemVueModelBuilder
{
	using CxStream = CircularBuffer<std::complex<double>>;
	using CxStreamBus = CircularBufferBusT<CxStream>;

    class SYSTEMVUEMODELBUILDER_API AsyncDistributorCx : public DFModel
	{
	public:
		AsyncDistributorCx();

		CxStream    input;   
		CxStreamBus output;  

		Matrix<int> BlockSizes;

		bool Setup() override;
		bool Run()   override;

		DECLARE_MODEL_INTERFACE(AsyncDistributorCx);

	private:
		std::vector<int> m_blockSizes;
	};
}
