#include "SubInt.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(SubInt)
{
    SET_MODEL_DESCRIPTION("Multiple Input Subtractor (int, self-defined)");
    SET_MODEL_SYMBOL("SYM_Sub");
    SET_MODEL_CATEGORY("Math/Arithmetic");

    ADD_MODEL_INPUT(pos);
    ADD_MODEL_INPUT(neg);
    ADD_MODEL_OUTPUT(output);
    return true;
}
#endif

SubInt::SubInt() {}

bool SubInt::Setup()
{
    pos.SetRate(1U);
    output.SetRate(1U);
    return true;
}

bool SubInt::Run()
{
    long long acc = static_cast<long long>(pos[0U]);
    const std::size_t M = neg.GetSize();
    for (std::size_t i = 0; i < M; ++i) {
        acc -= static_cast<long long>(neg[i][0U]);
    }
    output[0U] = static_cast<int>(acc);
    return true;
}
