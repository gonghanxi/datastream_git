#pragma once

#include "ModelBuilder.h"
#include "SystemVueModels.h"

namespace SystemVueModelBuilder
{
    class PAM_Demapper : public DFModel
	{
	public:
		CircularBuffer<double> input;      
		CircularBuffer<int>    Bits;       
		CircularBuffer<double> Amplitude;  

		int NumBits; 
		enum BitOrderE { LSBFirst, MSBFirst };
		BitOrderE BitOrder; 

		double HighLevel; 
		double LowLevel;  

		DECLARE_MODEL_INTERFACE(PAM_Demapper);

		PAM_Demapper();

		bool Setup();
		bool Initialize();
		bool Run();
		bool Finalize();

//	private:
		int    m_levels;   
		double m_step;     
		void   update_cache();
	};
}
