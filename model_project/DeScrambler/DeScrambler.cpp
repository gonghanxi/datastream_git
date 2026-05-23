#include "DeScrambler.h"

#ifndef SV_CODE_GEN

namespace SystemVueModelBuilder
{
	DEFINE_MODEL_INTERFACE(DeScrambler)
	{
		SET_MODEL_DESCRIPTION("Bit Sequence Descrambler");
		SET_MODEL_SYMBOL("SYM_DeScrambler");
		SET_MODEL_CATEGORY("Communications");

		ADD_MODEL_INPUT(input);
		ADD_MODEL_OUTPUT(output);

		{
			DFParam p = ADD_MODEL_PARAM(Polynomial);
			p.SetUnit(Units::NONE);
			p.SetDefaultValue("147457");
			p.SetDescription("Generator polynomial for the shift register- decimal, octal, or hex integer");
		}

		{
			DFParam p = ADD_MODEL_PARAM(ShiftReg);
			p.SetUnit(Units::NONE);
			p.SetDefaultValue("1");
			p.SetDescription("Initial state ofthe shift register -decimal, octal, or hex integer");
		}

		return true;
	}
}

#endif // SV_CODE_GEN


namespace SystemVueModelBuilder
{
	DeScrambler::DeScrambler()
		: Polynomial(147457),
		ShiftReg(1),
		mask(0),
		order(0),
		tapsMask(0U),
		state(0U)
	{
	}

	bool DeScrambler::Initialize()
	{
		return true;
	}

	bool DeScrambler::Setup()
	{
		if (Polynomial == 0)
		{
			POST_ERROR("Polynomial must be non-zero.");
			return false;
		}

		unsigned int poly = static_cast<unsigned int>(Polynomial);

		// low-order bit must be set
		if ((poly & 0x1U) == 0U)
		{
			POST_WARNING("Polynomial low-order bit (bit0) should be set; forcing bit0=1.");
			poly |= 0x1U;
		}

		order = highestSetBitIndex(poly);
		if (order < 0)
		{
			POST_ERROR("Polynomial is invalid (no set bits).");
			return false;
		}

		// Do not allow using sign bit (portable behavior similar to Keysight doc)
		const int maxOrder = static_cast<int>(8 * sizeof(int)) - 2; // e.g. 30 for 32-bit int
		if (order > maxOrder)
		{
			POST_ERROR("Polynomial order too large for 'int' implementation (sign bit not allowed).");
			return false;
		}

		if (order == 0)
		{
			mask = 0;
			tapsMask = 0U;
			state = 0U;
			return true;
		}

		mask = (1 << order) - 1;

		// Polynomial bit i (i>=1) corresponds to delay i -> state bit (i-1)
		tapsMask = (poly >> 1) & static_cast<unsigned int>(mask);

		// Initial delay-line state from ShiftReg (masked to 'order' bits)
		state = static_cast<unsigned int>(ShiftReg) & static_cast<unsigned int>(mask);

		return true;
	}

	bool DeScrambler::Run()
	{
		// input is boolean already; treat nonzero as 1
		const unsigned int inBit = input[0] ? 1U : 0U;

		// self-synchronizing descrambler (feed-forward):
		// out = in XOR parity(delayed-input taps)
		const unsigned int p = static_cast<unsigned int>(parity32(state & tapsMask));
		const unsigned int outBit = inBit ^ p;
		output[0] = (outBit != 0U);

		// update delay line: shift in CURRENT INPUT (not output)
		if (mask != 0)
		{
			state = ((state << 1) | inBit) & static_cast<unsigned int>(mask);
		}

		return true;
	}

	bool DeScrambler::Finalize()
	{
		return true;
	}

	// ---------- helper definitions (fix LNK2019) ----------
	int DeScrambler::parity32(unsigned int x)
	{
		// returns 0/1 parity
		x ^= x >> 16;
		x ^= x >> 8;
		x ^= x >> 4;
		x &= 0xFU;
		return (0x6996U >> x) & 1U;
	}

	int DeScrambler::highestSetBitIndex(unsigned int v)
	{
		if (v == 0U) return -1;
		int idx = 0;
		while (v >>= 1U) { ++idx; }
		return idx;
	}
}
