#include "AsyncCommutatorCx.h"

#include <iostream>
#include <sstream>

using namespace SystemVueModelBuilder;

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(AsyncCommutatorCx)
{
    SET_MODEL_DESCRIPTION("Asynchronous Data Commutator");
    SET_MODEL_SYMBOL("SYM_AsyncCommutator");
    SET_MODEL_CATEGORY("Routers/Resamplers");

    {
        auto p = ADD_MODEL_INPUT(input);
        p.SetName("input");
        p.SetDescription("bus of input data streams");
        p.SetOptional(true);
    }
    {
        auto p = ADD_MODEL_OUTPUT(output);
        p.SetName("output");
        p.SetDescription("output data stream");
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

AsyncCommutatorCx::AsyncCommutatorCx()
    : BlockSizes(1, 1)
{
    BlockSizes(0) = 1;
}

bool AsyncCommutatorCx::Setup()
{
    bool ok = true;

    const size_t numInputs = input.GetSize();
    size_t numOutputSamples = 0;

    if (numInputs != static_cast<size_t>(BlockSizes.NumElements())) {
        std::cout << "Size of BlockSizes array must be equal to number of inputs." << std::endl;
        return false;
    }

    for (size_t i = 0; i < numInputs; ++i) {
        const int Bi = BlockSizes(i);

        if (Bi > 0) {
            if (input[i].IsConnected()) {
                numOutputSamples += static_cast<size_t>(Bi);
                input[i].SetRate(static_cast<unsigned>(Bi));
            } else {
                std::stringstream ss;
                ss << "BlockSizes(" << (i + 1)
                   << ") should be 0 because input#" << (i + 1)
                   << " is a disconnected port";
                std::cout << ss.str() << std::endl;
                ok = false;
                break;
            }
        } else if (Bi == 0) {
            if (input[i].IsConnected()) {
                std::stringstream ss;
                ss << "BlockSizes(" << (i + 1)
                   << ") should be larger than 0 because input#" << (i + 1)
                   << " is a connected port";
                std::cout << ss.str() << std::endl;
                ok = false;
                break;
            }
        } else {
            std::cout << "Elements of BlockSizes array must all be >= 0." << std::endl;
            ok = false;
            break;
        }
    }

    if (!ok) {
        return false;
    }

    if (numOutputSamples == 0) {
        std::cout << "At least one element in the BlockSizes array must be larger than 0." << std::endl;
        return false;
    }

    output.SetRate(static_cast<unsigned>(numOutputSamples));

    return true;
}

bool AsyncCommutatorCx::Run()
{
    size_t k = 0;
    const size_t numInputs = input.GetSize();

    for (size_t i = 0; i < numInputs; ++i) {
        const int Bi = BlockSizes(i);
        if (Bi > 0) {
            input[i].Copy(0, &output, k, Bi);
            k += static_cast<size_t>(Bi);
        }
    }

    return true;
}
