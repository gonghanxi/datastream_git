#include "MpyInt.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(MpyInt)
{
    SET_MODEL_DESCRIPTION("Multiple Input Multiplier (int, self-defined)");
    SET_MODEL_SYMBOL("SYM_Mpy");
    SET_MODEL_CATEGORY("Math/Arithmetic");

    ADD_MODEL_INPUT(input);
    ADD_MODEL_OUTPUT(output);
    return true;
}
#endif

MpyInt::MpyInt() {}

bool MpyInt::Setup()
{
    output.SetRate(1U);
    return true;
}

bool MpyInt::Run()
{
    long long acc = 1;
    const std::size_t M = input.GetSize();
    for (std::size_t i = 0; i < M; ++i) {
        acc *= static_cast<long long>(input[i][0U]);
    }
    output[0U] = static_cast<int>(acc);
    return true;
}
