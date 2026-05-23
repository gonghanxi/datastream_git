#pragma once

#include "ModelBuilder.h"
#include "SystemVueModels.h"

namespace SystemVueModelBuilder
{
    class  GrayEncoder : public DFModel
	{
	public:
		CircularBuffer<bool> input;   
		CircularBuffer<bool> output;  

		int NumBits; 

		enum BitOrder { LSB_first, MSB_first };
		BitOrder m_BitOrder; 

		GrayEncoder();
		~GrayEncoder();

		bool Setup();
		bool Initialize();
		bool Run();
		bool Finalize();

		DECLARE_MODEL_INTERFACE(GrayEncoder);

//	private:
		bool* m_inBits;
		bool* m_outBits;

		bool EnsureBuffers();
		void FreeBuffers();

		void GrayEncodeBitsLSB0(const bool* binLSB0, bool* grayLSB0, int nbits);
	};
}
