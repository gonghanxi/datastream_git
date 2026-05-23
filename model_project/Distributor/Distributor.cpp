#include "Distributor.h"
#include <sstream>

using namespace SystemVueModelBuilder;

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(Distributor)
{
	SET_MODEL_DESCRIPTION("Synchronous Data Distributor");
	SET_MODEL_SYMBOL("SYM_Distributor");
	SET_MODEL_CATEGORY("Routers/Resamplers");

	{
		DFPort p = ADD_MODEL_INPUT(input);
		p.SetName("input");
		p.SetDescription("input data stream");
	}

	{
		DFPort p = ADD_MODEL_OUTPUT(output);
		p.SetName("output");
		p.SetDescription("bus of output data streams");
	}

	{
		DFParam par = ADD_MODEL_PARAM(BlockSize);
		par.SetName("BlockSize");
		par.SetDescription("Number of data items in a block");
		par.SetDefaultValue("1");   
	}

	return true;
}
#endif 

Distributor::Distributor()
	: BlockSize(1)
	, m_iBlockSize(1)
{
}

bool Distributor::Setup()
{
	if (BlockSize < 1)
		m_iBlockSize = 1;
	else
		m_iBlockSize = static_cast<size_t>(BlockSize);

	const size_t numOutputs = output.GetSize();

	for (size_t i = 0; i < numOutputs; ++i)
	{
		output[i].SetRate(static_cast<unsigned>(m_iBlockSize));
	}

	const size_t totalIn = m_iBlockSize * numOutputs;
	input.SetRate(static_cast<unsigned>(totalIn));

	return true;
}

bool Distributor::Run()
{
	const size_t numOutputs = output.GetSize();
	if (numOutputs == 0 || m_iBlockSize == 0)
		return true; 

	size_t j = 0; 

	for (size_t i = 0; i < numOutputs; ++i)
	{
		input.Copy(j, &output[i], 0, m_iBlockSize);
		j += m_iBlockSize;
	}

	return true;
}
