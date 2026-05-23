#include "SubCx.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(SubCx)
{
    SET_MODEL_DESCRIPTION("Multiple Input Subtractor (complex<double>)");
    SET_MODEL_SYMBOL("SYM_Sub");
    SET_MODEL_CATEGORY("Math Matrix");
    SET_MODEL_CATEGORY("Math Scalar");

    ADD_MODEL_INPUT(pos);
    ADD_MODEL_INPUT(neg);
    ADD_MODEL_OUTPUT(output);
    return true;
}
#endif

SubCx::SubCx() {}

bool SubCx::Setup()
{
    pos.SetRate(1U);
    output.SetRate(1U);
    return true;
}

bool SubCx::Run()
{
    cdouble acc = pos[0U];
    const std::size_t M = neg.GetSize();
    for (std::size_t i = 0; i < M; ++i) {
        acc -= neg[i][0U];
    }
    output[0U] = acc;
    return true;
}
