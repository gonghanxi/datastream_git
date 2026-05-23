#pragma once

#include "ModelBuilder.h"
#include "DFModel.h"
#include "CircularBuffer.h"

namespace SystemVueModelBuilder
{
	using DoubleStream = CircularBuffer<double>;
	using DoubleStreamBus = CircularBufferBusT<DoubleStream>;

    class SYSTEMVUEMODELBUILDER_API Distributor : public DFModel
	{
	public:
		Distributor();

		DoubleStream    input;   
		DoubleStreamBus output;  

		int BlockSize;

		bool Setup() override;
		bool Run()   override;

		DECLARE_MODEL_INTERFACE(Distributor);

	private:
		size_t m_iBlockSize;
	};

} 
