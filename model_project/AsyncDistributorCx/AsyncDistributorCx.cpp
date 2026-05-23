#include "AsyncDistributorCx.h"
#include <sstream>

using namespace SystemVueModelBuilder;

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(AsyncDistributorCx)
{
	SET_MODEL_DESCRIPTION("Asynchronous Data Distributor");
	SET_MODEL_SYMBOL("SYM_AsyncDistributor");
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
		DFParam par = ADD_MODEL_PARAM(BlockSizes);
		par.SetName("BlockSizes");
		par.SetDescription("Block sizes written to each output");
		par.SetDefaultValue("1");
	}

	return true;
}
#endif

AsyncDistributorCx::AsyncDistributorCx()
	: BlockSizes(1, 1)
{
	BlockSizes(0) = 1;
}

bool AsyncDistributorCx::Setup()
{
	const size_t numOutputs = output.GetSize();         
	const size_t numBlockVals = BlockSizes.NumElements(); 

	m_blockSizes.clear();

	if (numOutputs == 0)
		return true;  

	if (numBlockVals == 0)
	{
		POST_ERROR("AsyncDistributorCx: BlockSizes must contain at least one element.");
		return false;
	}

	m_blockSizes.resize(numOutputs);

	if (numBlockVals == 1)
	{
		int B = BlockSizes(0);
		if (B <= 0)
		{
			POST_ERROR("AsyncDistributorCx: elements of BlockSizes array must all be > 0.");
			return false;
		}

		for (size_t i = 0; i < numOutputs; ++i)
			m_blockSizes[i] = B;

		std::stringstream ss;
		ss << "AsyncDistributorCx: BlockSizes has 1 element, applying this value to all "
			<< numOutputs << " output(s).";
		POST_INFO(ss.str().c_str());
	}
	else if (numBlockVals >= numOutputs)
	{
		for (size_t i = 0; i < numOutputs; ++i)
		{
			int B = BlockSizes(i);
			if (B <= 0)
			{
				POST_ERROR("AsyncDistributorCx: elements of BlockSizes array must all be > 0.");
				return false;
			}
			m_blockSizes[i] = B;
		}

		if (numBlockVals > numOutputs)
		{
			std::stringstream ss;
			ss << "AsyncDistributorCx: BlockSizes has "
				<< numBlockVals << " element(s), but only "
				<< numOutputs << " output(s) are connected. "
				<< "Ignoring the extra elements.";
			POST_INFO(ss.str().c_str());
		}
	}
	else 
	{
		for (size_t i = 0; i < numBlockVals; ++i)
		{
			int B = BlockSizes(i);
			if (B <= 0)
			{
				POST_ERROR("AsyncDistributorCx: elements of BlockSizes array must all be > 0.");
				return false;
			}
			m_blockSizes[i] = B;
		}

		int lastB = BlockSizes(numBlockVals - 1);
		for (size_t i = numBlockVals; i < numOutputs; ++i)
			m_blockSizes[i] = lastB;

		std::stringstream ss;
		ss << "AsyncDistributorCx: BlockSizes has " << numBlockVals
			<< " element(s), but " << numOutputs
			<< " output(s) are connected. Reusing the last element for remaining outputs.";
		POST_INFO(ss.str().c_str());
	}

	size_t numInputData = 0;
	for (size_t i = 0; i < numOutputs; ++i)
	{
		int Bi = m_blockSizes[i];
		if (Bi <= 0)
		{
			POST_ERROR("AsyncDistributorCx: expanded BlockSizes element is not > 0.");
			return false;
		}

		numInputData += static_cast<size_t>(Bi);
		output[i].SetRate(static_cast<unsigned>(Bi));
	}

	if (numInputData == 0)
	{
		POST_ERROR("AsyncDistributorCx: total number of input samples per run must be > 0.");
		return false;
	}

	input.SetRate(static_cast<unsigned>(numInputData));
	return true;
}

bool AsyncDistributorCx::Run()
{
	const size_t numOutputs = output.GetSize();
	if (numOutputs == 0 || m_blockSizes.empty())
		return true;

	size_t k = 0; 

	for (size_t i = 0; i < numOutputs; ++i)
	{
		const int Bi = m_blockSizes[i];
		if (Bi > 0)
		{
			input.Copy(k, &output[i], 0, Bi);
			k += static_cast<size_t>(Bi);
		}
	}

	return true;
}
