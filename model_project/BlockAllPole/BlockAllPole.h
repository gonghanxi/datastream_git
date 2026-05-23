#pragma once

#include "ModelBuilder.h"
#include "CircularBuffer.h"
#include "SystemVue.h"
#include <vector>

namespace SystemVueModelBuilder {

    class SYSTEMVUEMODELBUILDER_API BlockAllPole : public DFModel
	{
	public:
		DECLARE_MODEL_INTERFACE(BlockAllPole);

		BlockAllPole();

		bool Setup() override;
		bool Run() override;

	protected:
		bool ValidateParameters();

	public:
		CircularBuffer<double> m_signalIn;
		CircularBuffer<double> m_coefs;

		CircularBuffer<double> m_signalOut;

	public:
		int m_blockSize;
		int m_order;

	protected:
		int m_delay;

		std::vector<double> m_taps;
		std::vector<double> m_delayLine;
	};

} 
