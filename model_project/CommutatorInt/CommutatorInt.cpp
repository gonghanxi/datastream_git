#include "CommutatorInt.h"

using namespace SystemVueModelBuilder;

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(CommutatorInt)
{
	SET_MODEL_DESCRIPTION("Synchronous Data Commutator");
	SET_MODEL_SYMBOL("SYM_Commutator");
	SET_MODEL_CATEGORY("Routers/Resamplers");

	ADD_MODEL_HEADER_FILE("SystemVue/Models/CommutatorInt.h");

	{
		auto p = ADD_MODEL_INPUT(input);
		p.SetName("input");
		p.SetDescription("bus of input data streams");
	}
	{
		auto p = ADD_MODEL_OUTPUT(output);
		p.SetName("output");
		p.SetDescription("output data stream");
	}
	{
		auto p = ADD_MODEL_PARAM(BlockSize);
		p.SetName("BlockSize");
		p.SetDescription("Number of particles in a block");
	}

	return true;
}
#endif

CommutatorInt::CommutatorInt()
	: BlockSize(1),
	m_iBlockSize(1u)
{
}

bool CommutatorInt::Setup()
{
	m_iBlockSize = (BlockSize < 1)
		? 1u
		: static_cast<size_t>(BlockSize);

	const size_t numInputs = input.GetSize();

	for (size_t i = 0; i < numInputs; ++i)
	{
		input[i].SetRate(static_cast<unsigned>(m_iBlockSize));
	}

	output.SetRate(static_cast<unsigned>(m_iBlockSize * numInputs));

	return true;
}

bool CommutatorInt::Run()
{
	const size_t numInputs = input.GetSize();
	size_t j = 0;

	for (size_t i = 0; i < numInputs; ++i)
	{
		input[i].Copy(0, &output, j, m_iBlockSize);
		j += m_iBlockSize;
	}

	return true;
}
