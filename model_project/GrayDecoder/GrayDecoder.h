#pragma once

#include "ModelBuilder.h"
#include "SystemVueModels.h"

namespace SystemVueModelBuilder
{
    class GrayDecoder : public DFModel
	{
	public:
		CircularBuffer<bool> input;   
		CircularBuffer<bool> output;  

		int NumBits; 

		enum BitOrderE { LSB_first, MSB_first };
		BitOrderE m_BitOrder;

		GrayDecoder();
		~GrayDecoder();

		bool Setup();
		bool Initialize();
		bool Run();
		bool Finalize();

		DECLARE_MODEL_INTERFACE(GrayDecoder);

//	private:
		bool* inBits;
		bool* outBits;

		void FreeBuffers();
		void AllocBuffers(int n);
	};
}
