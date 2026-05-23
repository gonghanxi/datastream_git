#pragma once

#include "ModelBuilder.h"
#include "DFModel.h"
#include "CircularBuffer.h"
#include <complex>

namespace SystemVueModelBuilder
{
	using Cx = std::complex<double>;
	using CxStream = CircularBuffer<Cx>;
	using CxBus = CircularBufferBusT<CxStream>;

    class SYSTEMVUEMODELBUILDER_API DistributorCx : public DFModel
	{
	public:
		DistributorCx();

		CxStream input;   
		CxBus    output;  

		int BlockSize;

		bool Setup() override;
		bool Run()   override;

		DECLARE_MODEL_INTERFACE(DistributorCx);

	private:
		size_t m_iBlockSize;
	};

} 
