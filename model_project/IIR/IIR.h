#pragma once
#include "SystemVue.h"
#include "ModelBuilder.h"
#include <cfloat>
#include <cmath>

namespace SystemVueModelBuilder
{
    class SYSTEMVUEMODELBUILDER_API IIR : public DFModel
	{
	public:
		IIR();
		virtual ~IIR();

		DECLARE_MODEL_INTERFACE(IIR);

		bool Initialize() override;
		bool Run()        override;
		bool Finalize()   override;

		CircularBuffer<double> m_input;
		CircularBuffer<double> m_output;

		double  m_Gain;          
		double* m_Numerator;     
		double* m_Denominator;   
		int     m_iNumeratorSize;
		int     m_iDenominatorSize;

	private:
		int     m_iNumState;
		double* m_State;
	};

} 
