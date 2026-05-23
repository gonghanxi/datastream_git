#include "AddInt.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(AddInt)
{
    SET_MODEL_DESCRIPTION("Multiple Input Adder (int, self-defined)");
    SET_MODEL_SYMBOL("SYM_Add");
    SET_MODEL_CATEGORY("Math/Arithmetic");

    ADD_MODEL_INPUT(input);
    ADD_MODEL_OUTPUT(output);
    return true;
}
#endif

AddInt::AddInt() {}

bool AddInt::Setup()
{
    output.SetRate(1U);
    return true;
}

bool AddInt::Run()
{
    long long acc = 0;
    const std::size_t M = input.GetSize();
    for (std::size_t i = 0; i < M; ++i) {
        acc += static_cast<long long>(input[i][0U]);
    }
    output[0U] = static_cast<int>(acc);
    return true;
}
