#pragma once
#include "SystemVue.h"
#include "ModelBuilder.h"
#include <cstddef>   
#include <cmath>     
#include <cfloat>    

namespace SystemVueModelBuilder
{
    class SYSTEMVUEMODELBUILDER_API BiquadCascade : public DFModel
	{
	public:
		BiquadCascade();
		virtual ~BiquadCascade();

		DECLARE_MODEL_INTERFACE(BiquadCascade);

		bool Initialize() override;
		bool Run()        override;
		bool Finalize()   override;

		DoubleCircularBuffer    m_dInput;
		DoubleCircularBufferBus m_dOutput;

		double* m_pdTaps;
		int     m_iTapsSize;

	private:
		double* m_pdState1;   
		double* m_pdState2;   

		struct BiquadBlock
		{
			double b0, b1, b2;   
			double a1, a2;       
		};

		BiquadBlock* m_pBlocks;    
		std::size_t  m_iNumBiquads;
	};

} 
