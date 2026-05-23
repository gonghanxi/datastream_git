#include "BitShiftRegister.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(BitShiftRegister)
{
    SET_MODEL_DESCRIPTION("Bit shift register");
    SET_MODEL_SYMBOL("SYM_BitShiftRegister");
    SET_MODEL_CATEGORY("Digital/Logic");

    {
        SystemVueModelBuilder::DFPort p = ADD_MODEL_INPUT(input);
        p.SetDescription("Input bits");
    }
    {
        SystemVueModelBuilder::DFPort p = ADD_MODEL_INPUT(clock);
        p.SetDescription("Clock signal");
        p.SetOptional();
    }
    {
        SystemVueModelBuilder::DFPort p = ADD_MODEL_INPUT(reset);
        p.SetDescription("Reset signal");
        p.SetOptional();
    }
    {
        SystemVueModelBuilder::DFPort p = ADD_MODEL_OUTPUT(output);
        p.SetDescription("Output bit register");
    }

    {
        SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(NumBits);
        p.SetDescription("Number of bits in the output register");
        p.SetDefaultValue("8");
    }

    {
        SystemVueModelBuilder::DFParam p =
            ADD_MODEL_ENUM_PARAM(BitOrder, BitShiftRegister::BitOrderEnum);
        p.SetDescription("Output bit order");
        p.AddEnumeration("LSB first", BitShiftRegister::LSB_FIRST);
        p.AddEnumeration("MSB first", BitShiftRegister::MSB_FIRST);
        p.SetDefaultValue("MSB first");
    }

    return true;
}
#endif

BitShiftRegister::BitShiftRegister()
    : NumBits(8)
    , BitOrder(MSB_FIRST)
{
}

bool BitShiftRegister::Setup()
{
    if (NumBits <= 0)
        NumBits = 1;

    output.SetRate(static_cast<unsigned int>(NumBits));

    reg_.assign(static_cast<std::size_t>(NumBits), 0);
    return true;
}

bool BitShiftRegister::Run()
{
    bool inBool = false;
    if (input.IsConnected())
        inBool = input[0];

    int inBit = toBit(inBool);

    int clk = 1;
    int rst = 0;

    if (clock.IsConnected())
        clk = clock[0];

    if (reset.IsConnected())
        rst = reset[0];

    if (rst != 0)
    {
        std::fill(reg_.begin(), reg_.end(), 0);
    }
    else if (!clock.IsConnected() || clk != 0)
    {
        const int N = static_cast<int>(reg_.size());
        if (N > 0)
        {
            for (int i = N - 1; i > 0; --i)
                reg_[i] = reg_[i - 1];

            reg_[0] = inBit;
        }
    }

    const int Nbits = static_cast<int>(reg_.size());
    if (Nbits <= 0)
        return true;

    if (BitOrder == LSB_FIRST)
    {
        for (int i = 0; i < Nbits; ++i)
            output[static_cast<unsigned int>(i)] = (reg_[i] != 0);
    }
    else
    {
        for (int i = 0; i < Nbits; ++i)
            output[static_cast<unsigned int>(i)] =
            (reg_[Nbits - 1 - i] != 0);
    }

    return true;
}
