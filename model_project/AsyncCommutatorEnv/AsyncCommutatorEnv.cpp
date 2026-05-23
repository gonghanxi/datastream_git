#include "AsyncCommutatorEnv.h"

using namespace SystemVueModelBuilder;

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(AsyncCommutatorEnv)
{
	SET_MODEL_DESCRIPTION("Asynchronous Data Commutator (Envelope)");
	SET_MODEL_SYMBOL("SYM_AsyncCommutator");
	SET_MODEL_CATEGORY("Routers/Resamplers");

	{
		auto p = ADD_MODEL_INPUT(input);
		p.SetName("input");
		p.SetDescription("bus of input envelope data streams");
		p.SetOptional(true);
	}

	{
		auto p = ADD_MODEL_OUTPUT(output);
		p.SetName("output");
		p.SetDescription("output envelope data stream");
	}

	{
		auto p = ADD_MODEL_PARAM(BlockSizes);
		p.SetName("BlockSizes");
		p.SetDefaultValue("1");
		p.SetDescription("Block sizes read from each input");
	}

	return true;
}
#endif

AsyncCommutatorEnv::AsyncCommutatorEnv()
	: BlockSizes(1, 1),
	m_fcOut(0.0)
{
	BlockSizes(0) = 1;
}

static bool FcEqual(double a, double b)
{
	const double scale = std::max({ 1.0, std::fabs(a), std::fabs(b) });
	return (std::fabs(a - b) <= 1e-12 * scale);
}

bool AsyncCommutatorEnv::Setup()
{
	bool ok = true;

	const size_t numInputs = input.GetSize();
	size_t numOutputSamples = 0;

	if (numInputs != static_cast<size_t>(BlockSizes.NumElements()))
	{
		POST_ERROR("Size of BlockSizes array must be equal to number of inputs.");
		return false;
	}

	bool fcInitialized = false;
	m_fcOut = 0.0;

	for (size_t i = 0; i < numInputs; ++i)
	{
		const int Bi = BlockSizes(i);

		if (Bi > 0)
		{
			if (input[i].IsConnected())
			{
				numOutputSamples += static_cast<size_t>(Bi);
				input[i].SetRate(static_cast<unsigned>(Bi));

				const double fcIn = input[i].GetCharacterizationFrequency();
				if (!fcInitialized)
				{
					m_fcOut = fcIn;
					fcInitialized = true;
				}
				else
				{
					if (!FcEqual(fcIn, m_fcOut))
					{
						std::stringstream ss;
						ss << "Characterization frequency mismatch: input#"
							<< (i + 1) << " fc=" << fcIn
							<< " differs from output fc=" << m_fcOut
							<< ". All active inputs (Bi>0) must share the same characterization frequency.";
						POST_ERROR(ss.str().c_str());
						ok = false;
						break;
					}
				}
			}
			else
			{
				std::stringstream ss;
				ss << "BlockSizes(" << (i + 1)
					<< ") should be 0 because input#" << (i + 1)
					<< " is a disconnected port";
				POST_ERROR(ss.str().c_str());
				ok = false;
				break;
			}
		}
		else if (Bi == 0)
		{
			if (input[i].IsConnected())
			{
				std::stringstream ss;
				ss << "BlockSizes(" << (i + 1)
					<< ") should be larger than 0 because input#" << (i + 1)
					<< " is a connected port";
				POST_ERROR(ss.str().c_str());
				ok = false;
				break;
			}
		}
		else
		{
			POST_ERROR("Elements of BlockSizes array must all be >= 0.");
			ok = false;
			break;
		}
	}

	if (!ok)
		return false;

	if (numOutputSamples == 0)
	{
		POST_ERROR("At least one element in the BlockSizes array must be larger than 0.");
		return false;
	}

	output.SetRate(static_cast<unsigned>(numOutputSamples));
	output.SetCharacterizationFrequency(m_fcOut);

	return true;
}

bool AsyncCommutatorEnv::Run()
{
	size_t k = 0;
	const size_t numInputs = input.GetSize();

	for (size_t i = 0; i < numInputs; ++i)
	{
		const int Bi = BlockSizes(i);
		if (Bi > 0)
		{
			for (int j = 0; j < Bi; ++j)
			{
				output[static_cast<unsigned>(k + static_cast<size_t>(j))] = input[i][static_cast<unsigned>(j)];
			}
			k += static_cast<size_t>(Bi);
		}
	}

	return true;
}
