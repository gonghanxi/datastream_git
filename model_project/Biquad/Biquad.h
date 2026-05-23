#pragma once

#include "ModelBuilder.h"
#include "SystemVue.h"

namespace SystemVueModelBuilder
{
    class SYSTEMVUEMODELBUILDER_API Biquad : public DFModel
	{
	public:
		Biquad();
		virtual ~Biquad() {}

		DECLARE_MODEL_INTERFACE(Biquad);

		bool Initialize() override;
		bool Run()        override;
		bool Finalize()   override;

		CircularBuffer<double> m_dInput;
		CircularBuffer<double> m_dOutput;

		double m_dD1;
		double m_dD2;
		double m_dN0;
		double m_dN1;
		double m_dN2;

	private:
		double m_dState1;
		double m_dState2;
	};
} 
