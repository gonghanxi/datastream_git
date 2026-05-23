#pragma once

#include "ModelBuilder.h"
#include "SystemVueModels.h"

namespace SystemVueModelBuilder
{
    class DeScrambler : public DFModel
	{
	public:
		CircularBuffer<bool> input;   // input bit sequence (zero or nonzero)
		CircularBuffer<bool> output;  // output bit sequence (zero or one)

		CircularBuffer<int> stepNumber; // (kept as provided)

		// Parameters
		int Polynomial; // Generator polynomial for the shift register - decimal, octal, or hex integer
		int ShiftReg;   // Initial state of the shift register - decimal, octal, or hex integer

		DeScrambler();

		bool Initialize();
		bool Setup();
		bool Run();
		bool Finalize();

		DECLARE_MODEL_INTERFACE(DeScrambler);

//	protected:
		int mask;               
		int order;              
		unsigned int tapsMask;  
		unsigned int state;     

		static int parity32(unsigned int x);
		static int highestSetBitIndex(unsigned int v);
	};
}
