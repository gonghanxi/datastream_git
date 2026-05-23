#include "BitsToInt.h"
#include <iostream>

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(BitsToInt)
{
    SET_MODEL_DESCRIPTION("Bits to Integer Converter");
    SET_MODEL_SYMBOL("SYM_BitsToInt");
    SET_MODEL_CATEGORY("Math Scalar");
    SET_MODEL_CATEGORY("Type Converters");

    ADD_MODEL_INPUT(input);
    ADD_MODEL_OUTPUT(output);

    {
        SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(NumBits);
        p.SetUnit(SystemVueModelBuilder::Units::NONE);
        p.SetDefaultValue("4");
        p.SetDescription("Number of bits read per execution");
    }

    {
        SystemVueModelBuilder::DFParam e =
            ADD_MODEL_ENUM_PARAM(BitOrder, BitOrderEnum);
        e.SetUnit(SystemVueModelBuilder::Units::NONE);
        e.AddEnumeration("LSB first", LSB_first);
        e.AddEnumeration("MSB first", MSB_first);
        e.SetDefaultValue("MSB first");
        e.SetDescription("Bit order");
    }

    return true;
}
#endif

BitsToInt::BitsToInt()
    : NumBits(4), BitOrder(MSB_first)
{}

bool BitsToInt::Setup()
{
    if (NumBits < 1 || NumBits > 32) {
        std::cout << "NumBits must be in [1, 32]." << std::endl;
        return false;
    }

    input.SetRate(static_cast<unsigned>(NumBits));
    output.SetRate(1U);
    return true;
}

bool BitsToInt::Run()
{
    const int N = NumBits;
    uint32_t acc = 0u;

    if (BitOrder == LSB_first) {
        for (int i = 0; i < N; ++i) {
            const bool b = input[static_cast<unsigned>(i)];
            acc |= (static_cast<uint32_t>(b ? 1u : 0u) << i);
        }
    }
    else {
        for (int i = 0; i < N; ++i) {
            const bool b = input[static_cast<unsigned>(i)];
            acc = (acc << 1) | (b ? 1u : 0u);
        }
    }

    int out_val = 0;
    if (N == 32) {
        const int64_t val =
            (acc & 0x80000000u) ? (static_cast<int64_t>(acc) - 0x1'0000'0000LL)
            : static_cast<int64_t>(acc);
        out_val = static_cast<int>(val);
    }
    else {
        out_val = static_cast<int>(acc);
    }

    output[0U] = out_val;
    return true;
}
